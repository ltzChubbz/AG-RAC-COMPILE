#include "wad.h"
#include "decompress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

static char* find_wad_path(const char *requested_path) {
    static char final_path[512];
    const char *prefixes[] = { "", "../", "../../", "../../../", "" };
    
    for (int i = 0; i < 5; i++) {
        snprintf(final_path, sizeof(final_path), "%s%s", prefixes[i], requested_path);
        if (access(final_path, F_OK) == 0) {
            return final_path;
        }
    }
    return NULL;
}

WadFile* wad_load(const char *path) {
    printf("[WAD] Loading %s...\n", path);
    fflush(stdout);
    char *actual_path = find_wad_path(path);
    if (!actual_path) {
        fprintf(stderr, "[WAD] CRITICAL ERROR: Could not find WAD file at %s (checked parent dirs)\n", path);
        return NULL;
    }

    FILE *f = fopen(actual_path, "rb");
    if (!f) {
        fprintf(stderr, "[WAD] Failed to open %s\n", actual_path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    u32 size = (u32)ftell(f);
    fseek(f, 0, SEEK_SET);

    WadFile *wad = calloc(1, sizeof(WadFile));
    wad->data = malloc(size);
    wad->size = size;
    fread(wad->data, 1, size, f);
    fclose(f);

    /* 
     * Stateful Chained Decompression (Phase 13)
     * We decompress all WAD blocks sequentially into a single buffer.
     * The first 0x2800 bytes (BOOT ELF) are used as the initial seed.
     */
    wad->decompressed_size = 0x2800;
    wad->decompressed_data = malloc(8 * 1024 * 1024); /* Allocate 8MB for decompressed level */
    memcpy(wad->decompressed_data, wad->data, 0x2800);

    for (u32 i = 0; i < size; i += 2048) {
        if (size - i >= 16 && memcmp(&wad->data[i], "WAD", 3) == 0) {
            u32 written = 0;
            decompress_wad_block(&wad->data[i], wad->data + size, wad->decompressed_data, wad->decompressed_size, &written);
            printf("[WAD] Decompressed block at 0x%X -> %u bytes (total: %u)\n", i, written, wad->decompressed_size + written);
            wad->decompressed_size += written;
            fflush(stdout);
        }
    }

    /* Search for LevelCoreHeader in the decompressed buffer */
    wad->core_header_offset = 0xFFFFFFFF;
    for (u32 i = 0; i < wad->decompressed_size - 64; i++) {
        /* Signature: [gs_ram_count=32][gs_ram_offset=5040] */
        u32 count = *(u32*)&wad->decompressed_data[i];
        u32 offset = *(u32*)&wad->decompressed_data[i+4];
        if (count == 32 && offset == 5040) {
            u32 tfrags_off = *(u32*)&wad->decompressed_data[i+8];
            u32 occlusion_off = *(u32*)&wad->decompressed_data[i+12];
            u32 sky_off = *(u32*)&wad->decompressed_data[i+16];
            u32 collision_off = *(u32*)&wad->decompressed_data[i+20];
            
            /* If this looks like the dummy MIPS header or has invalid sky offset, skip it */
            if (sky_off > 0x1000000) {
                continue;
            }
            
            wad->core_header_offset = i;
            printf("[WAD] Found LevelCoreHeader at 0x%X:\n", i);
            printf("      gs_ram: count=%u, offset=0x%X\n", count, offset);
            printf("      tfrags: offset=0x%X\n", tfrags_off);
            printf("      occlusion: offset=0x%X\n", occlusion_off);
            printf("      sky: offset=0x%X\n", sky_off);
            printf("      collision: offset=0x%X\n", collision_off);
            fflush(stdout);
            break;
        }
    }

    /* Decompress embedded core_data WAD block */
    wad->core_data = NULL;
    wad->core_data_size = 0;
    if (wad->core_header_offset != 0xFFFFFFFF) {
        u32 embedded_wad_offset = 0;
        for (u32 j = wad->core_header_offset + 0x10; j < wad->decompressed_size - 4; j++) {
            if (memcmp(&wad->decompressed_data[j], "WAD", 3) == 0) {
                embedded_wad_offset = j;
                break;
            }
        }
        
        if (embedded_wad_offset != 0) {
            u32 seed_size = embedded_wad_offset - wad->core_header_offset;
            printf("[WAD] Found embedded core_data WAD block at 0x%X (seed size: %u bytes)\n", 
                   embedded_wad_offset, seed_size);
            fflush(stdout);
            
            // Allocate 8MB for decompressed core_data (including seed)
            wad->core_data = malloc(8 * 1024 * 1024);
            memcpy(wad->core_data, &wad->decompressed_data[wad->core_header_offset], seed_size);
            
            u32 core_data_decomp_size = 0;
            decompress_wad_block(&wad->decompressed_data[embedded_wad_offset], 
                                 &wad->decompressed_data[wad->decompressed_size], 
                                 wad->core_data, 
                                 seed_size, 
                                 &core_data_decomp_size);
            
            wad->core_data_size = seed_size + core_data_decomp_size;
            printf("[WAD] Decompressed embedded core_data -> %u bytes (total including seed: %u)\n", 
                   core_data_decomp_size, wad->core_data_size);
            fflush(stdout);
        } else {
            printf("[WAD] WARNING: Could not find embedded core_data WAD block inside Block B!\n");
            fflush(stdout);
        }
    }

    /* 
     * Scan for Geometry Chunks (uncompressed VIF packets)
     * R&C streams geometry directly from DVD without WAD compression.
     */
    u32 max_chunks = size / 2048 + 1;
    wad->chunks = malloc(sizeof(WadChunk) * max_chunks);
    wad->chunk_count = 0;
    
    for (u32 i = 0; i < size; i += 2048) {
        if (size - i >= 16 && memcmp(&wad->data[i], "WAD", 3) == 0) {
            continue; // Skip compressed blocks
        }
        
        int is_geom = 0;
        u32 scan_end = (i + 2048 <= size) ? 2048 : (size - i);
        for (u32 j = 0; j < scan_end - 4; j += 4) {
            u8 cmd = wad->data[i + j + 3];
            u8 num = wad->data[i + j + 2];
            if ((cmd & 0x60) == 0x60) {
                u8 v_type = (cmd >> 2) & 0x03;
                u8 d_type = cmd & 0x03;
                if (v_type == 3 && d_type == 1 && num > 0 && num < 255) {
                    is_geom = 1;
                    break;
                }
            }
        }
        
        if (is_geom) {
            wad->chunks[wad->chunk_count].offset = i;
            wad->chunks[wad->chunk_count].size = 2048;
            memcpy(wad->chunks[wad->chunk_count].type, "GEOM", 4);
            wad->chunk_count++;
        }
    }
    
    printf("[WAD] Found %u geometry chunks in %s\n", wad->chunk_count, actual_path);

    if (wad->core_header_offset == 0xFFFFFFFF) {
        printf("[WAD] WARNING: Could not find RacLevelDataHeader in decompressed stream!\n");
        fflush(stdout);
    }

    printf("[WAD] Successfully loaded %s (%u MB raw, %u MB decompressed)\n", 
           actual_path, size / (1024*1024), wad->decompressed_size / (1024*1024));
    fflush(stdout);
    return wad;
}

void wad_free(WadFile *wad) {
    if (!wad) return;
    if (wad->data) free(wad->data);
    if (wad->decompressed_data) free(wad->decompressed_data);
    if (wad->core_data) free(wad->core_data);
    if (wad->chunks) free(wad->chunks);
    free(wad);
}

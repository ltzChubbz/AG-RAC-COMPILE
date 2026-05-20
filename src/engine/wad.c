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
            decompress_wad_block(&wad->data[i], wad->decompressed_data, wad->decompressed_size, &written);
            printf("[WAD] Decompressed block at 0x%X -> %u bytes (total: %u)\n", i, written, wad->decompressed_size + written);
            wad->decompressed_size += written;
            fflush(stdout);
        }
    }

    /* Search for RacLevelDataHeader in the decompressed buffer */
    wad->core_header_offset = 0xFFFFFFFF;
    for (u32 i = 0; i < wad->decompressed_size - 64; i++) {
        /* Signature: [gs_ram_count=32][gs_ram_offset=5040] */
        u32 count = *(u32*)&wad->decompressed_data[i];
        u32 offset = *(u32*)&wad->decompressed_data[i+4];
        if (count == 32 && offset == 5040) {
            wad->core_header_offset = i;
            
            /* Print discovered sub-offsets */
            u32 ci_off = *(u32*)&wad->decompressed_data[i+0x10];
            u32 ci_size = *(u32*)&wad->decompressed_data[i+0x14];
            printf("[WAD] Found RacLevelDataHeader at 0x%X (core_index: 0x%X, size: 0x%X)\n", i, ci_off, ci_size);
            fflush(stdout);
            
            /* If this looks like the dummy MIPS header, keep searching */
            if (ci_off > 0x1000000) {
                printf("[WAD] Skipping dummy header...\n");
                wad->core_header_offset = 0xFFFFFFFF;
                continue;
            }
            u32 ci_abs = i + ci_off;
            if (ci_abs < wad->decompressed_size - 32) {
                u32 tfrags_off = *(u32*)&wad->decompressed_data[ci_abs+8];
                u32 sky_off = *(u32*)&wad->decompressed_data[ci_abs+16];
                printf("[WAD] LevelCoreHeader at 0x%X (tfrags: 0x%X, sky: 0x%X)\n", ci_abs, tfrags_off, sky_off);
                fflush(stdout);
            }
            break;
        }
    }

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
    if (wad->chunks) free(wad->chunks);
    free(wad);
}

#ifndef AG_RAC_ENGINE_WAD_H
#define AG_RAC_ENGINE_WAD_H

#include "types.h"

typedef struct WadChunk {
    u32 offset;
    u32 size;
    char type[4];
} WadChunk;

typedef struct WadFile {
    char name[64];
    u8 *data;
    u32 size;
    
    /* Decompressed Level Data */
    u8 *decompressed_data;
    u32 decompressed_size;
    u32 core_header_offset; /* Offset to RacLevelDataHeader in decompressed_data */

    WadChunk *chunks;
    u32 chunk_count;
} WadFile;

/**
 * wad_load() — Load a deinterleaved WAD file into memory and scan for chunks.
 */
WadFile* wad_load(const char *path);

/**
 * wad_free() — Cleanup WAD memory.
 */
void wad_free(WadFile *wad);

#endif /* AG_RAC_ENGINE_WAD_H */

#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include "types.h"

/**
 * Decompresses a WAD-style compressed block.
 * @param src Pointer to the "WAD" magic.
 * @param dest The destination buffer (must be large enough).
 * @param dest_size Current size of data in dest (used for lookback).
 * @param out_size Pointer to store the number of bytes written.
 * @return Pointer to the next byte after the compressed block in src.
 */
u8* decompress_wad_block(u8* src, u8* src_end, u8* dest, u32 dest_size, u32* out_size);

#endif

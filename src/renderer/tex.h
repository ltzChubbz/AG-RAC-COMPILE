#ifndef AG_RAC_RENDERER_TEX_H
#define AG_RAC_RENDERER_TEX_H

#include "types.h"

/**
 * tex_from_wad() — Convert PS2 CLUT+Pixels to an OpenGL RGBA texture.
 * clut: 1024 bytes of 256-color palette (RGBA).
 * pixels: width * height bytes of 8-bit indices.
 */
u32 tex_from_wad(const u8 *clut, const u8 *pixels, u32 width, u32 height);

#endif /* AG_RAC_RENDERER_TEX_H */

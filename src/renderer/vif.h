#ifndef AG_RAC_VIF_H
#define AG_RAC_VIF_H

#include "types.h"

typedef struct VifVertex {
    f32 x, y, z;
    f32 u, v;
    u16 flag;
} VifVertex;

/**
 * Parses a 2048-byte WAD sector as a VIF stream.
 * Returns the number of vertices found.
 */
u32 vif_parse_sector(const u8 *data, u32 size, VifVertex *out_verts, u32 max_verts);

#endif /* AG_RAC_VIF_H */

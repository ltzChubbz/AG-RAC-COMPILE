#ifndef AG_RAC_RENDERER_MESH_H
#define AG_RAC_RENDERER_MESH_H

#include "types.h"
#include "../../src/engine/wad.h"

typedef struct PcMesh {
    u32 vao;
    u32 vbo;
    u32 ebo;
    u32 index_count;
    u32 vert_count;
    int initialized;
} PcMesh;

/**
 * mesh_from_chunk() — Create a GPU-ready mesh from a WAD chunk.
 * This translates PS2 fixed-point vertices into PC floating-point.
 */
PcMesh* mesh_from_chunk(const WadChunk *chunk, const u8 *wad_data);

/**
 * mesh_from_tfrag() — Create a GPU-ready mesh from a TFrag VIF payload.
 */
PcMesh* mesh_from_tfrag(const u8 *vif_data, u32 vif_size);

/**
 * mesh_draw() — Render the mesh to the screen.
 */
void mesh_draw(PcMesh *mesh, float *view_matrix, float cam_x, float cam_y, float cam_z);

#endif /* AG_RAC_RENDERER_MESH_H */

/**
 * vu.h — PS2 Vector Unit (VU0 / VU1) HAL
 *
 * VU0: Used as a COP2 co-processor for physics / matrix math in game code.
 * VU1: Fed via DMA, runs geometry pipeline microcode.
 *
 * On PC, VU0 intrinsics become SSE/AVX SIMD calls.
 * VU1 microcode programs become software geometry routines (translated separately).
 */

#ifndef AG_RAC_HAL_VU_H
#define AG_RAC_HAL_VU_H

#include "../../include/types.h"
#include "../../include/ps2_math.h"

/* ── VU0 Operations (COP2 replacement) ────────────────────────────────────── */
/* These match the PS2 VU0 instruction semantics for decompiled game code. */

/** vu0_outer_product_w — VU0 OPMSUB: (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x) */
static inline Vec3 vu0_cross_product(Vec3 a, Vec3 b) {
    return vec3_cross(a, b);
}

/** vu0_dot — VU0 DP (XYZ dot product into W result) */
static inline f32 vu0_dot3(Vec3 a, Vec3 b) {
    return vec3_dot(a, b);
}

/** vu0_sqrt — VU0 SQRT (square root of W component) */
static inline f32 vu0_sqrt(f32 x) {
    return rac_sqrtf(x);
}

/** vu0_rsqrt — VU0 RSQRT (reciprocal square root) */
static inline f32 vu0_rsqrt(f32 x) {
    return 1.0f / rac_sqrtf(x);
}

/** vu0_mul_matrix — VU0 MUL: matrix * matrix */
static inline Mat4 vu0_mul_matrix(Mat4 a, Mat4 b) {
    return mat4_mul(a, b);
}

/** vu0_apply_matrix — VU0: Apply 4×4 matrix to a Vec4 */
static inline Vec4 vu0_apply_matrix(Mat4 m, Vec4 v) {
    return mat4_mul_vec4(m, v);
}

/* ── VU1 Program Interface ─────────────────────────────────────────────────── */

/**
 * vu1_upload_program() — "Upload" a VU1 microcode program.
 *   On PS2, VU1 microcode is written to VU1 instruction memory via DMA.
 *   On PC, we register a C function that will be called to emulate it.
 *
 *   @param prog_id    Unique program identifier
 *   @param func       PC function that implements this VU1 program's behavior
 */
typedef void (*Vu1Program)(const void *input_data, u32 vertex_count, void *output_gif_data);
void vu1_upload_program(u32 prog_id, Vu1Program func);

/**
 * vu1_kick() — Execute the currently loaded VU1 program.
 *   On PS2, this writes the START bit in the VIF1 register.
 *   On PC, this calls the registered Vu1Program function.
 *
 *   @param prog_id    Which program to run
 *   @param data       Input data (vertex buffer, matrix palette, etc.)
 *   @param count      Vertex count
 *   @param out        Output GIF packet buffer
 */
void vu1_kick(u32 prog_id, const void *data, u32 count, void *out);

#endif /* AG_RAC_HAL_VU_H */

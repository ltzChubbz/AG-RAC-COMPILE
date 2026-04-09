/**
 * ps2_math.h — PS2 math intrinsics compatibility header
 *
 * On PS2, many math operations used COP2 (VU0 as co-processor) instructions
 * or the EE's custom MMI (Multimedia Instructions) via inline assembly.
 *
 * On PC, we replace these with SSE2/AVX intrinsics or standard C math.
 * Decompiled code should call these wrappers rather than using
 * platform-specific builtins directly.
 */

#ifndef AG_RAC_PS2_MATH_H
#define AG_RAC_PS2_MATH_H

#include "types.h"
#include <math.h>
#include <string.h>  /* memcpy */

#ifdef TARGET_PC
#include <immintrin.h>  /* SSE/AVX intrinsics */
#endif

/* ── Scalar math ───────────────────────────────────────────────────────────── */

static inline f32 rac_sqrtf(f32 x)         { return sqrtf(x); }
static inline f32 rac_fabsf(f32 x)         { return fabsf(x); }
static inline f32 rac_sinf(f32 x)          { return sinf(x); }
static inline f32 rac_cosf(f32 x)          { return cosf(x); }
static inline f32 rac_atan2f(f32 y, f32 x) { return atan2f(y, x); }

static inline f32 rac_clampf(f32 v, f32 lo, f32 hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline s32 rac_clampi(s32 v, s32 lo, s32 hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline f32 rac_lerpf(f32 a, f32 b, f32 t) {
    return a + t * (b - a);
}

/* ── Vec3 operations ───────────────────────────────────────────────────────── */

static inline Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Vec3 vec3_scale(Vec3 v, f32 s) {
    return (Vec3){ v.x * s, v.y * s, v.z * s };
}

static inline f32 vec3_dot(Vec3 a, Vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline f32 vec3_length_sq(Vec3 v) {
    return vec3_dot(v, v);
}

static inline f32 vec3_length(Vec3 v) {
    return rac_sqrtf(vec3_length_sq(v));
}

static inline Vec3 vec3_normalize(Vec3 v) {
    f32 inv_len = 1.0f / vec3_length(v);
    return vec3_scale(v, inv_len);
}

static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x,
    };
}

static inline Vec3 vec3_lerp(Vec3 a, Vec3 b, f32 t) {
    return (Vec3){
        rac_lerpf(a.x, b.x, t),
        rac_lerpf(a.y, b.y, t),
        rac_lerpf(a.z, b.z, t),
    };
}

/* Negate */
static inline Vec3 vec3_neg(Vec3 v) {
    return (Vec3){ -v.x, -v.y, -v.z };
}

/* ── Vec4 operations ───────────────────────────────────────────────────────── */

static inline Vec4 vec4_add(Vec4 a, Vec4 b) {
    return (Vec4){ a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w };
}

static inline f32 vec4_dot(Vec4 a, Vec4 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

/* ── Mat4 operations ───────────────────────────────────────────────────────── */

/* Identity matrix */
static inline Mat4 mat4_identity(void) {
    Mat4 m = {{{0}}};
    m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
    return m;
}

/* Matrix × matrix */
static inline Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 result = {{{0}}};
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            f32 sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a.m[k][row] * b.m[col][k];
            }
            result.m[col][row] = sum;
        }
    }
    return result;
}

/* Matrix × Vec4 */
static inline Vec4 mat4_mul_vec4(Mat4 m, Vec4 v) {
    return (Vec4){
        m.m[0][0]*v.x + m.m[1][0]*v.y + m.m[2][0]*v.z + m.m[3][0]*v.w,
        m.m[0][1]*v.x + m.m[1][1]*v.y + m.m[2][1]*v.z + m.m[3][1]*v.w,
        m.m[0][2]*v.x + m.m[1][2]*v.y + m.m[2][2]*v.z + m.m[3][2]*v.w,
        m.m[0][3]*v.x + m.m[1][3]*v.y + m.m[2][3]*v.z + m.m[3][3]*v.w,
    };
}

/* Transform a Vec3 position (w=1) */
static inline Vec3 mat4_transform_point(Mat4 m, Vec3 p) {
    Vec4 r = mat4_mul_vec4(m, (Vec4){p.x, p.y, p.z, 1.0f});
    return (Vec3){ r.x, r.y, r.z };
}

/* Transform a Vec3 direction (w=0, ignores translation) */
static inline Vec3 mat4_transform_dir(Mat4 m, Vec3 d) {
    Vec4 r = mat4_mul_vec4(m, (Vec4){d.x, d.y, d.z, 0.0f});
    return (Vec3){ r.x, r.y, r.z };
}

#endif /* AG_RAC_PS2_MATH_H */

/**
 * math.c — Engine Math Library Implementation
 *
 * STATUS: STUB — these are placeholder implementations.
 * The REAL implementations will be produced by matching decompilation:
 *   1. Identify each function in Ghidra
 *   2. Write C code that compiles to byte-identical MIPS output
 *   3. Verify with asm-differ and SHA1 comparison
 *   4. Replace stub with matched implementation
 *
 * Once matched, remove the STUB comment from each function.
 */

#include "math.h"
#include <math.h>

/* ── M_PI may not be defined on all platforms ────────────────────────────── */
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* ── Scalar Math ─────────────────────────────────────────────────────────── */

f32 ClampFloat(f32 value, f32 lo, f32 hi) {
    /* STUB */
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

s32 ClampInt(s32 value, s32 lo, s32 hi) {
    /* STUB */
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

f32 Abs(f32 x) {
    /* STUB */
    return (x < 0.0f) ? -x : x;
}

f32 Lerp(f32 a, f32 b, f32 t) {
    /* STUB */
    return a + t * (b - a);
}

f32 WrapAngle(f32 angle) {
    /* STUB */
    while (angle >  (f32)M_PI) angle -= 2.0f * (f32)M_PI;
    while (angle < -(f32)M_PI) angle += 2.0f * (f32)M_PI;
    return angle;
}

/* ── Vector Math ─────────────────────────────────────────────────────────── */

Vec3 Vec3Normalize(Vec3 v) {
    /* STUB */
    f32 len_sq = v.x*v.x + v.y*v.y + v.z*v.z;
    if (len_sq < 1e-10f) {
        return (Vec3){0.0f, 0.0f, 0.0f};
    }
    f32 inv_len = 1.0f / sqrtf(len_sq);
    return (Vec3){ v.x * inv_len, v.y * inv_len, v.z * inv_len };
}

f32 Vec3Dist(Vec3 a, Vec3 b) {
    /* STUB */
    Vec3 diff = { a.x - b.x, a.y - b.y, a.z - b.z };
    return sqrtf(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
}

f32 Vec3DistSq(Vec3 a, Vec3 b) {
    /* STUB */
    Vec3 diff = { a.x - b.x, a.y - b.y, a.z - b.z };
    return diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
}

/* ── Matrix Math ─────────────────────────────────────────────────────────── */

Mat4 Mat4FromEulerYXZ(f32 yaw, f32 pitch, f32 roll) {
    /* STUB */
    f32 sy = sinf(yaw),   cy = cosf(yaw);
    f32 sp = sinf(pitch), cp = cosf(pitch);
    f32 sr = sinf(roll),  cr = cosf(roll);

    Mat4 m = {{{0}}};
    /* YXZ Euler decomposition */
    m.m[0][0] =  cy*cr + sy*sp*sr;
    m.m[1][0] = -cy*sr + sy*sp*cr;
    m.m[2][0] =  sy*cp;

    m.m[0][1] =  cp*sr;
    m.m[1][1] =  cp*cr;
    m.m[2][1] = -sp;

    m.m[0][2] = -sy*cr + cy*sp*sr;
    m.m[1][2] =  sy*sr + cy*sp*cr;
    m.m[2][2] =  cy*cp;

    m.m[3][3] = 1.0f;
    return m;
}

Mat4 Mat4Translation(Vec3 t) {
    /* STUB */
    Mat4 m = {{{0}}};
    m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
    m.m[3][0] = t.x;
    m.m[3][1] = t.y;
    m.m[3][2] = t.z;
    return m;
}

Mat4 Mat4Scale(Vec3 s) {
    /* STUB */
    Mat4 m = {{{0}}};
    m.m[0][0] = s.x;
    m.m[1][1] = s.y;
    m.m[2][2] = s.z;
    m.m[3][3] = 1.0f;
    return m;
}

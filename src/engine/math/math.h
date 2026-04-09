/**
 * math.h — Engine Math Library
 *
 * This is the decompiled C source for R&C's internal math utility functions.
 * These functions live in the .text segment of the main ELF.
 *
 * Status: STUB — awaiting matching decompilation from Ghidra analysis.
 *
 * Decompilation target:
 *   Original binary: extracted/SCUS_971.99
 *   Section: .text
 *   Address range: TBD (to be identified via Ghidra)
 *
 * The functions here are declared based on known game behavior (e.g., observed
 * via PCSX2 debugger). Their implementations in math.c must match the original
 * MIPS assembly byte-for-byte when compiled with mipsel-linux-gnu-gcc.
 */

#ifndef RAC_ENGINE_MATH_H
#define RAC_ENGINE_MATH_H

#include "../../include/types.h"
#include "../../include/ps2_math.h"

/* ── Scalar Math ───────────────────────────────────────────────────────────── */

/**
 * Clamp a float to the range [lo, hi].
 * Address: TBD
 */
f32 ClampFloat(f32 value, f32 lo, f32 hi);

/**
 * Clamp an integer to the range [lo, hi].
 * Address: TBD
 */
s32 ClampInt(s32 value, s32 lo, s32 hi);

/**
 * Return the absolute value of a float.
 * Address: TBD
 */
f32 Abs(f32 x);

/**
 * Linear interpolation between a and b by factor t (0..1).
 * Address: TBD
 */
f32 Lerp(f32 a, f32 b, f32 t);

/**
 * Wrap angle (in radians) to the range [-PI, PI].
 * Address: TBD
 */
f32 WrapAngle(f32 angle);

/* ── Vector Math ───────────────────────────────────────────────────────────── */

/**
 * Normalize a Vec3. Returns zero vector if input is near zero.
 * Address: TBD
 */
Vec3 Vec3Normalize(Vec3 v);

/**
 * Distance between two points.
 * Address: TBD
 */
f32 Vec3Dist(Vec3 a, Vec3 b);

/**
 * Distance squared between two points (cheaper, avoids sqrt).
 * Address: TBD
 */
f32 Vec3DistSq(Vec3 a, Vec3 b);

/* ── Matrix Math ───────────────────────────────────────────────────────────── */

/**
 * Build a 4×4 rotation matrix from Euler angles (in radians, YXZ order).
 * The PS2 games typically use YXZ Euler order for character transforms.
 * Address: TBD
 */
Mat4 Mat4FromEulerYXZ(f32 yaw, f32 pitch, f32 roll);

/**
 * Build a translation matrix.
 * Address: TBD
 */
Mat4 Mat4Translation(Vec3 t);

/**
 * Build a scale matrix.
 * Address: TBD
 */
Mat4 Mat4Scale(Vec3 s);

#endif /* RAC_ENGINE_MATH_H */

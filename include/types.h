/**
 * types.h — AG-RAC base type definitions
 *
 * Canonical type aliases matching the PS2 SDK (SN Systems ProDG) conventions.
 * All decompiled C source code uses these types exclusively.
 *
 * On the PS2 (MIPS R5900, 32-bit):
 *   char  = 8-bit
 *   short = 16-bit
 *   int   = 32-bit
 *   long  = 32-bit (MIPS O32 ABI — NOT 64-bit!)
 *
 * On x86-64 PC:
 *   These map to fixed-width types from <stdint.h>
 */

#ifndef AG_RAC_TYPES_H
#define AG_RAC_TYPES_H

#include <stdint.h>
#include <stddef.h>

/* ── Integer types ─────────────────────────────────────────────────────────── */

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;

typedef float     f32;
typedef double    f64;

/* ── Boolean ───────────────────────────────────────────────────────────────── */

typedef s32  bool32;   /* PS2 style boolean — 0 or 1 in a 32-bit int */

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* ── Vector types ──────────────────────────────────────────────────────────── */

/* 3-component float vector — position, velocity, scale, etc. */
typedef struct Vec3 {
    f32 x, y, z;
} Vec3;

/* 4-component float vector — homogeneous coords, RGBA color, quaternion */
typedef struct Vec4 {
    f32 x, y, z, w;
} Vec4;

/* 2-component float vector — UV texture coordinates */
typedef struct Vec2 {
    f32 u, v;
} Vec2;

/* 3-component integer vector */
typedef struct IVec3 {
    s32 x, y, z;
} IVec3;

/* ── Matrix types ──────────────────────────────────────────────────────────── */

/* 4×4 column-major float matrix (matches PS2 VU convention) */
typedef struct Mat4 {
    f32 m[4][4];
} Mat4;

/* ── Color types ───────────────────────────────────────────────────────────── */

/* PS2 RGBA — alpha is 0–128 (not 0–255) on the GS! */
typedef struct Color4 {
    u8 r, g, b, a;
} Color4;

/* 32-bit packed color (GS native format) */
typedef u32 GsColor;

/* ── Pointer-sized integer ─────────────────────────────────────────────────── */

/* PS2 addresses are 32-bit. On PC we use uintptr_t. */
typedef uintptr_t uptr;
typedef  intptr_t sptr;

/* ── Alignment macro ───────────────────────────────────────────────────────── */

/* Align a value up to the given power-of-2 boundary */
#define ALIGN_UP(val, align)   (((val) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(val, align) ((val) & ~((align) - 1))

/* ── Array size helper ─────────────────────────────────────────────────────── */

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* ── Unused parameter suppression ─────────────────────────────────────────── */

#define UNUSED(x) ((void)(x))

/* ── Platform function attributes ─────────────────────────────────────────── */

/* Functions that match PS2 SDK linkage should be annotated if needed */
#define GAME_FUNC   /* no-op on PC */
#define LEVEL_FUNC  /* function present in level overlay ELF */

#endif /* AG_RAC_TYPES_H */

/**
 * gs.h — PS2 Graphics Synthesizer (GS) Hardware Abstraction Layer
 *
 * The GS is the PS2's rasterizer. It accepts GIFtag packets (128-bit
 * drawing commands) from VU1 via the GIF interface, and rasterizes
 * primitives into its 4 MB eDRAM frame buffer.
 *
 * On PC, GS calls are translated to OpenGL 4.6 draw calls.
 * This header provides:
 *   1. GS register definitions (matching PS2 hardware addresses)
 *   2. GIFtag structure definitions
 *   3. PC-side HAL functions that translate GS operations to OpenGL
 */

#ifndef AG_RAC_HAL_GS_H
#define AG_RAC_HAL_GS_H

#include "../../include/types.h"

/* ── GS Privileged Register Addresses ─────────────────────────────────────── */
/* On PS2 these are memory-mapped at 0x12000000. On PC they are structs. */

#define GS_REG_PMODE    0x12000000  /* Readout circuit mode */
#define GS_REG_SMODE2   0x12000020  /* Sync mode (NTSC/PAL) */
#define GS_REG_DISPFB1  0x120000A0  /* Display frame buffer 1 */
#define GS_REG_DISPLAY1 0x120000C0  /* Display 1 settings */
#define GS_REG_CSR      0x12001000  /* System status/control */

/* ── GS Drawing Registers (accessed via GIFtag PACKED or REGLIST mode) ─────── */

#define GS_REG_PRIM     0x00  /* Primitive type and render mode */
#define GS_REG_RGBAQ    0x01  /* Vertex color (R, G, B, A, Q) */
#define GS_REG_ST       0x02  /* Texture coordinates (S, T) */
#define GS_REG_UV       0x03  /* Texture coordinates (integer U, V) */
#define GS_REG_XYZF2    0x04  /* Vertex (X, Y, Z, fog) — auto draw */
#define GS_REG_XYZ2     0x05  /* Vertex (X, Y, Z) — auto draw */
#define GS_REG_TEX0_1   0x06  /* Texture settings buffer 1 */
#define GS_REG_TEX1_1   0x14  /* Texture filter settings buffer 1 */
#define GS_REG_ALPHA_1  0x42  /* Alpha blend settings buffer 1 */
#define GS_REG_ZBUF_1   0x4E  /* Z-buffer settings buffer 1 */
#define GS_REG_FRAME_1  0x4C  /* Frame buffer settings buffer 1 */
#define GS_REG_SCISSOR_1 0x40 /* Scissor rectangle buffer 1 */
#define GS_REG_TEST_1   0x47  /* Pixel test settings buffer 1 */

/* ── GS Primitive Types ────────────────────────────────────────────────────── */

typedef enum GsPrimType {
    GS_PRIM_POINT        = 0,
    GS_PRIM_LINE         = 1,
    GS_PRIM_LINE_STRIP   = 2,
    GS_PRIM_TRIANGLE     = 3,
    GS_PRIM_TRIANGLE_STRIP = 4,
    GS_PRIM_TRIANGLE_FAN = 5,
    GS_PRIM_SPRITE       = 6,   /* Axis-aligned rectangle (billboard) */
} GsPrimType;

/* ── GS Alpha Blend Modes ──────────────────────────────────────────────────── */

typedef enum GsAlphaMode {
    GS_ALPHA_SRC_COLOR = 0,
    GS_ALPHA_DST_COLOR = 1,
    GS_ALPHA_ZERO      = 2,
} GsAlphaMode;

/* ── GIFtag ────────────────────────────────────────────────────────────────── */
/* A 128-bit packet header for GIF (Graphics Interface) data transfers */

typedef struct GifTag {
    u64 lo;  /* NLOOP, EOP, PRE, PRIM, FLG, NREG */
    u64 hi;  /* REGS (4-bit × 16 register descriptors) */
} GifTag;

/* GIFtag FLG field — data format */
#define GIF_FLG_PACKED  0  /* PACKED mode: 128-bit entries with reg descriptors */
#define GIF_FLG_REGLIST 1  /* REGLIST mode: 64-bit entries, regs from tag */
#define GIF_FLG_IMAGE   2  /* IMAGE mode: raw texture data */
#define GIF_FLG_IMAGE2  3  /* IMAGE2 mode (unused in R&C) */

/* ── Frame Buffer Configuration ────────────────────────────────────────────── */

typedef struct GsFrameConfig {
    u32 base_ptr;     /* Frame buffer base pointer (in GS eDRAM words) */
    u32 width;        /* Buffer width in pixels (must be power of 2) */
    u32 psm;          /* Pixel storage format (e.g., PSMCT32) */
    u32 draw_mask;    /* Write mask — bits set = write disabled */
} GsFrameConfig;

/* Pixel format codes */
#define GS_PSM_PSMCT32  0x00  /* 32bpp RGBA */
#define GS_PSM_PSMCT24  0x01  /* 24bpp RGB */
#define GS_PSM_PSMCT16  0x02  /* 16bpp */
#define GS_PSM_PSMT8    0x13  /* 8bpp palettized */
#define GS_PSM_PSMT4    0x14  /* 4bpp palettized */
#define GS_PSM_PSMZ32   0x30  /* 32bpp Z-buffer */
#define GS_PSM_PSMZ16   0x32  /* 16bpp Z-buffer */

/* ── HAL Public API ────────────────────────────────────────────────────────── */

/**
 * gs_hal_init() — Initialize the GS HAL (sets up OpenGL context state).
 *   Must be called once after OpenGL context creation.
 *   @param width   Render resolution width
 *   @param height  Render resolution height
 */
void gs_hal_init(u32 width, u32 height);

/**
 * gs_hal_shutdown() — Clean up GS HAL resources.
 */
void gs_hal_shutdown(void);

/**
 * gs_hal_begin_frame() — Called at the start of each frame.
 *   Clears the color and depth buffers.
 */
void gs_hal_begin_frame(void);

/**
 * gs_hal_end_frame() — Called at the end of each frame.
 *   Swap buffers / present.
 */
void gs_hal_end_frame(void);

/**
 * gs_hal_set_alpha() — Configure alpha blending state.
 *   Maps GS ALPHA register to glBlendFunc/glBlendEquation.
 */
void gs_hal_set_alpha(u8 a, u8 b, u8 c, u8 d, u8 fix);

/**
 * gs_hal_set_test() — Configure pixel test state (alpha test, depth test, etc.)
 */
void gs_hal_set_test(u32 test_reg_val);

/**
 * gs_hal_upload_texture() — Upload a texture to GPU VRAM.
 *   @param data    Pixel data (in PS2 swizzled format — will be un-swizzled)
 *   @param width   Texture width
 *   @param height  Texture height
 *   @param psm     PS2 pixel format code
 *   @param clut    CLUT data for palettized formats (NULL for direct color)
 *   @return        OpenGL texture ID
 */
u32 gs_hal_upload_texture(const u8 *data, u32 width, u32 height,
                           u32 psm, const u8 *clut);

/**
 * gs_hal_process_gifpath3() — Process a GIF path 3 packet (DMA-fed geometry).
 *   The game's VU1 microcode builds GIFtag packets; this function interprets
 *   them and issues the equivalent OpenGL draw calls.
 *
 *   @param data    Pointer to the GIF packet data (128-bit aligned)
 *   @param size    Size of data in bytes
 */
void gs_hal_process_gifpath3(const void *data, u32 size);

#endif /* AG_RAC_HAL_GS_H */

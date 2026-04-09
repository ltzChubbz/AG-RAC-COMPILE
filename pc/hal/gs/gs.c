/**
 * gs.c — PS2 GS HAL Implementation (stub)
 *
 * This is the PC-side implementation of the GS Hardware Abstraction Layer.
 * It translates GS register writes and GIFtag packet processing into
 * OpenGL 4.6 state changes and draw calls.
 *
 * STATUS: Stub — functions are defined but not yet implemented.
 *         GS packet decoding and OpenGL translation will be implemented
 *         as we decompile the renderer subsystem.
 */

#include "gs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TARGET_PC

/* Included via vcpkg or system OpenGL */
/* #include <GL/gl.h>    -- Will be enabled once OpenGL deps are configured */
/* #include <SDL2/SDL.h> -- Will be enabled once SDL2 deps are configured */

/* ── Internal State ─────────────────────────────────────────────────────────── */

typedef struct GsHalState {
    u32 render_width;
    u32 render_height;
    u32 initialized;

    /* Current GS register state (mirrors PS2 GS MMRs) */
    u32  current_psm;        /* Pixel format */
    u8   alpha_a, alpha_b;   /* Blend source */
    u8   alpha_c, alpha_d;   /* Blend destination */

    /* Texture cache — maps GS texture base pointer to GL texture ID */
    /* TODO: implement hash map once decompilation begins */
} GsHalState;

static GsHalState g_gs = {0};

/* ── Public API Implementation ─────────────────────────────────────────────── */

void gs_hal_init(u32 width, u32 height) {
    if (g_gs.initialized) {
        fprintf(stderr, "[GS-HAL] WARNING: gs_hal_init called twice\n");
        return;
    }

    g_gs.render_width  = width;
    g_gs.render_height = height;
    g_gs.initialized   = 1;

    /*
     * TODO: Initialize OpenGL state:
     *   glEnable(GL_DEPTH_TEST);
     *   glDepthFunc(GL_LEQUAL);
     *   glEnable(GL_BLEND);
     *   glViewport(0, 0, width, height);
     */

    printf("[GS-HAL] Initialized — %ux%u (stub)\n", width, height);
}

void gs_hal_shutdown(void) {
    if (!g_gs.initialized) return;

    /*
     * TODO: Free GL resources (textures, FBOs, shader programs)
     */

    memset(&g_gs, 0, sizeof(g_gs));
    printf("[GS-HAL] Shutdown\n");
}

void gs_hal_begin_frame(void) {
    /*
     * TODO:
     *   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
     *   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
     */
}

void gs_hal_end_frame(void) {
    /*
     * TODO:
     *   SDL_GL_SwapWindow(window);
     */
}

void gs_hal_set_alpha(u8 a, u8 b, u8 c, u8 d, u8 fix) {
    g_gs.alpha_a = a;
    g_gs.alpha_b = b;
    g_gs.alpha_c = c;
    g_gs.alpha_d = d;

    /*
     * PS2 GS alpha blend formula:
     *   output = ((A - B) * C >> 7) + D
     * Where A, B, D select from: Cs (src color), Cd (dst color), 0
     * And C selects from: As (src alpha), Ad (dst alpha), FIX (const)
     *
     * TODO: Map this to glBlendFuncSeparate / glBlendEquationSeparate
     */
    (void)fix;
}

void gs_hal_set_test(u32 test_reg_val) {
    /*
     * The TEST register packs alpha test, destination alpha test,
     * and depth test settings into a 64-bit value.
     *
     * TODO: Parse individual fields and map to:
     *   glEnable/glDisable(GL_ALPHA_TEST)  -- deprecated in GL 3.3+, use shader
     *   glEnable/glDisable(GL_DEPTH_TEST)
     *   glDepthFunc(GL_LESS / GL_LEQUAL / etc.)
     */
    (void)test_reg_val;
}

u32 gs_hal_upload_texture(const u8 *data, u32 width, u32 height,
                           u32 psm, const u8 *clut) {
    /*
     * TODO:
     * 1. Un-swizzle PS2 texture data (PSMT8/PSMT4 use a peculiar block swizzle)
     * 2. Un-swizzle CLUT if palettized
     * 3. Convert to RGBA8 for GL upload
     * 4. glGenTextures / glBindTexture / glTexImage2D
     * 5. Return GL texture ID
     */
    (void)data; (void)width; (void)height; (void)psm; (void)clut;
    return 0;  /* Stub: return invalid texture ID */
}

void gs_hal_process_gifpath3(const void *data, u32 size) {
    /*
     * Process a stream of GIF packets. Each packet starts with a 128-bit
     * GIFtag header, followed by vertex data.
     *
     * GIFtag layout (lo 64 bits):
     *   bits  0-14  NLOOP  — number of data units following
     *   bit   15    EOP    — end of packet flag
     *   bit   46    PRE    — if set, PRIM field is loaded into GS PRIM reg
     *   bits 47-57  PRIM   — primitive settings
     *   bits 58-59  FLG    — data format (PACKED/REGLIST/IMAGE)
     *   bits 60-63  NREG   — number of registers in REGS field
     *
     * GIFtag layout (hi 64 bits):
     *   Four 4-bit register descriptors × up to 16 registers
     *
     * TODO: Implement GIF packet parser and dispatch to OpenGL.
     */
    (void)data; (void)size;
}

#endif /* TARGET_PC */

/**
 * window.c — PC Window and Main Loop Implementation (stub)
 *
 * SDL2 window creation, OpenGL context setup, and main game loop.
 * This is the true entry point (main()) for the PC build.
 */

#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TARGET_PC

/* #include <SDL2/SDL.h>        -- Enabled once SDL2 is configured */
/* #include <SDL2/SDL_opengl.h> -- Enabled once SDL2/GL is configured */

/* ── Internal State ─────────────────────────────────────────────────────────── */

typedef struct WindowState {
    /* SDL_Window   *window;    -- SDL2 window handle */
    /* SDL_GLContext gl_ctx;    -- OpenGL context */
    int    width;
    int    height;
    int    running;
    float  delta_time;
    /* Uint32 last_frame_ticks; -- SDL tick count at start of last frame */
} WindowState;

static WindowState g_window = {0};

/* ── Public API ──────────────────────────────────────────────────────────────── */

int window_init(int scale) {
    g_window.width  = RAC_NATIVE_WIDTH  * scale;
    g_window.height = RAC_NATIVE_HEIGHT * scale;
    g_window.running = 1;

    printf("[WINDOW] Initializing %dx%d window (scale %dx)\n",
           g_window.width, g_window.height, scale);

    /*
     * TODO: SDL2 initialization sequence:
     *
     * if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
     *     fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
     *     return -1;
     * }
     *
     * // Request OpenGL 4.6 Core Profile
     * SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
     * SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
     * SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
     * SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
     * SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
     *
     * g_window.window = SDL_CreateWindow(
     *     RAC_WINDOW_TITLE,
     *     SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
     *     g_window.width, g_window.height,
     *     SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
     * );
     * if (!g_window.window) {
     *     fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
     *     return -1;
     * }
     *
     * g_window.gl_ctx = SDL_GL_CreateContext(g_window.window);
     * if (!g_window.gl_ctx) {
     *     fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
     *     return -1;
     * }
     *
     * // Enable vsync
     * SDL_GL_SetSwapInterval(1);
     */

    /* Initialize all HAL systems */
    gs_hal_init((u32)g_window.width, (u32)g_window.height);
    pad_hal_init();

    /* Use the ASSETS_PATH define from CMake */
#ifdef ASSETS_PATH
    iop_hal_init(ASSETS_PATH);
#else
    iop_hal_init("assets");
#endif

    printf("[WINDOW] Initialization complete (stub)\n");
    return 0;
}

void window_shutdown(void) {
    iop_hal_shutdown();
    pad_hal_shutdown();
    gs_hal_shutdown();

    /*
     * TODO:
     * SDL_GL_DeleteContext(g_window.gl_ctx);
     * SDL_DestroyWindow(g_window.window);
     * SDL_Quit();
     */

    memset(&g_window, 0, sizeof(g_window));
    printf("[WINDOW] Shutdown complete\n");
}

void window_run(void) {
    printf("[WINDOW] Starting main loop\n");

    /* Call game initialization */
    GameInit();

    while (g_window.running) {
        /*
         * TODO: Frame timing:
         * Uint32 frame_start = SDL_GetTicks();
         */

        /* ── Event pump ─────────────────────────────────────────────────── */
        /*
         * TODO:
         * SDL_Event event;
         * while (SDL_PollEvent(&event)) {
         *     if (event.type == SDL_QUIT) {
         *         window_request_quit();
         *     }
         *     if (event.type == SDL_KEYDOWN) {
         *         if (event.key.keysym.sym == SDLK_ESCAPE) {
         *             window_request_quit();
         *         }
         *         if (event.key.keysym.sym == SDLK_F11) {
         *             // Toggle fullscreen
         *             SDL_SetWindowFullscreen(g_window.window,
         *                 SDL_GetWindowFlags(g_window.window) & SDL_WINDOW_FULLSCREEN_DESKTOP
         *                 ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
         *         }
         *     }
         * }
         */

        /* ── Update ─────────────────────────────────────────────────────── */
        pad_hal_update();
        GameUpdate();

        /* ── Render ─────────────────────────────────────────────────────── */
        gs_hal_begin_frame();
        GameRender();
        gs_hal_end_frame();

        /*
         * TODO: Frame rate cap:
         * Uint32 frame_time = SDL_GetTicks() - frame_start;
         * if (frame_time < RAC_FRAME_TIME_MS) {
         *     SDL_Delay(RAC_FRAME_TIME_MS - frame_time);
         * }
         * g_window.delta_time = (SDL_GetTicks() - frame_start) / 1000.0f;
         */

        /* Stub: break immediately since game functions aren't implemented yet */
        printf("[WINDOW] Frame executed (stub — exiting immediately)\n");
        g_window.running = 0;
    }

    GameShutdown();
    printf("[WINDOW] Main loop exited\n");
}

void window_request_quit(void) {
    g_window.running = 0;
}

float window_get_delta_time(void) {
    return g_window.delta_time;
}

/* ── PC Entry Point ──────────────────────────────────────────────────────────── */

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    printf("\n");
    printf("  AG-RAC — Ratchet & Clank Native PC Port\n");
    printf("  =========================================\n");
    printf("  Build: PC (Development stub)\n");
    printf("\n");

    int scale = RAC_WINDOW_SCALE;
    if (argc > 1) {
        scale = atoi(argv[1]);
        if (scale < 1 || scale > 8) scale = RAC_WINDOW_SCALE;
    }

    if (window_init(scale) != 0) {
        fprintf(stderr, "[ERROR] window_init failed\n");
        return 1;
    }

    window_run();
    window_shutdown();

    return 0;
}

/* ── Stub game function implementations (until decomp provides real ones) ─── */

void GameInit(void) {
    printf("[GAME] GameInit() — stub\n");
}

void GameUpdate(void) {
    /* Stub: nothing to update yet */
}

void GameRender(void) {
    /* Stub: nothing to render yet */
}

void GameShutdown(void) {
    printf("[GAME] GameShutdown() — stub\n");
}

#endif /* TARGET_PC */

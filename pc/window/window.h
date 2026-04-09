/**
 * window.h — PC Window, Main Loop, and SDL2 Integration
 *
 * Manages the application window, OpenGL context, main game loop timing,
 * and SDL event pump. This is the PC entry point that replaces the PS2's
 * BIOS-managed startup sequence.
 */

#ifndef AG_RAC_PC_WINDOW_H
#define AG_RAC_PC_WINDOW_H

#include "../hal/gs/gs.h"
#include "../hal/pad/pad.h"
#include "../hal/iop/iop.h"

/* Default render resolution (PS2 native: 512×448 NTSC) */
#define RAC_NATIVE_WIDTH  512
#define RAC_NATIVE_HEIGHT 448

/* Default PC window scale factor */
#define RAC_WINDOW_SCALE  3    /* 512×448 × 3 = 1536×1344 */

/* Window title */
#define RAC_WINDOW_TITLE  "AG-RAC — Ratchet & Clank"

/* Target frame rate (PS2 ran at 60 Hz NTSC) */
#define RAC_TARGET_FPS  60
#define RAC_FRAME_TIME_MS  (1000 / RAC_TARGET_FPS)

/* ── Window API ────────────────────────────────────────────────────────────── */

/**
 * window_init() — Initialize SDL2, create window, create OpenGL context.
 *   @param scale   Window scale multiplier (1 = native PS2 resolution)
 *   @return        0 on success, -1 on failure
 */
int window_init(int scale);

/**
 * window_shutdown() — Destroy window, GL context, and SDL2.
 */
void window_shutdown(void);

/**
 * window_run() — Run the main game loop until the user quits.
 *   Calls the game's update and render functions each frame.
 *   Internally calls:
 *     pad_hal_update()
 *     GameUpdate()       ← decompiled game logic
 *     gs_hal_begin_frame()
 *     GameRender()       ← decompiled renderer
 *     gs_hal_end_frame()
 */
void window_run(void);

/**
 * window_request_quit() — Signal the main loop to exit cleanly.
 *   Can be called from game code (e.g., "Exit to OS" menu option).
 */
void window_request_quit(void);

/**
 * window_get_delta_time() — Get time since last frame in seconds.
 *   Used to scale time-based game logic on PC (PS2 code often
 *   assumes fixed 60 Hz — this helps identify such code during decompilation).
 */
float window_get_delta_time(void);

/* ── Forward declarations of game functions (defined in src/) ──────────────── */

/**
 * GameInit() — Game initialization entry point.
 *   On PS2, this is called after the ELF loads and the IOP modules are ready.
 *   On PC, this is called after all HAL systems are initialized.
 */
extern void GameInit(void);

/**
 * GameUpdate() — Called once per frame for logic updates.
 */
extern void GameUpdate(void);

/**
 * GameRender() — Called once per frame for rendering.
 */
extern void GameRender(void);

/**
 * GameShutdown() — Called on clean exit.
 */
extern void GameShutdown(void);

#endif /* AG_RAC_PC_WINDOW_H */

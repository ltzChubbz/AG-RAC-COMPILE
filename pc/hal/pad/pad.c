/**
 * pad.c — PS2 PAD HAL Implementation (stub)
 *
 * Translates SDL2 gamepad events into PS2 DualShock 2 controller format.
 */

#include "pad.h"
#include <stdio.h>
#include <string.h>

#ifdef TARGET_PC

/* #include <SDL2/SDL.h>  -- Enabled once SDL2 is configured */

/* ── Internal State ─────────────────────────────────────────────────────────── */

#define MAX_PORTS 2

typedef struct PadHalState {
    PadData current[MAX_PORTS];
    PadData previous[MAX_PORTS];
    int     sdl_joy_index[MAX_PORTS];  /* SDL2 joystick index, -1 if disconnected */
    int     initialized;
} PadHalState;

static PadHalState g_pad = {0};

/* Deadzone for analog sticks (out of 128) */
#define ANALOG_DEADZONE 10

/* ── Internal helpers ───────────────────────────────────────────────────────── */

static u8 axis_to_ps2(int sdl_axis_value) {
    /*
     * SDL2 axis range: -32768 to 32767
     * PS2 range: 0–255, center = 128
     */
    int mapped = (sdl_axis_value / 258) + 128;
    if (mapped < 0)   mapped = 0;
    if (mapped > 255) mapped = 255;
    return (u8)mapped;
}

/* ── Public API ──────────────────────────────────────────────────────────────── */

void pad_hal_init(void) {
    memset(&g_pad, 0, sizeof(g_pad));
    for (int i = 0; i < MAX_PORTS; i++) {
        g_pad.sdl_joy_index[i] = -1;
        /* Start with all buttons "not pressed" (active-low → all 1s) */
        g_pad.current[i].buttons  = 0xFFFF;
        g_pad.previous[i].buttons = 0xFFFF;
        g_pad.current[i].mode     = 0x7;  /* DualShock2 mode */
        /* Center analog sticks */
        g_pad.current[i].lx = g_pad.current[i].ly = 128;
        g_pad.current[i].rx = g_pad.current[i].ry = 128;
    }
    g_pad.initialized = 1;

    /*
     * TODO: SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
     *       Open available game controllers, assign to ports.
     */
    printf("[PAD-HAL] Initialized (stub)\n");
}

void pad_hal_shutdown(void) {
    if (!g_pad.initialized) return;
    /*
     * TODO: SDL_GameControllerClose for all open controllers
     */
    memset(&g_pad, 0, sizeof(g_pad));
}

void pad_hal_update(void) {
    /*
     * TODO: For each SDL2 GameController:
     *
     * 1. Save previous state:
     *    memcpy(&g_pad.previous[port], &g_pad.current[port], sizeof(PadData));
     *
     * 2. Read SDL button states and map to PS2 bitmask (active-low):
     *    u16 buttons = 0xFFFF;  // all not-pressed
     *    if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_A))
     *        buttons &= ~PAD_CROSS;   // Clear bit = pressed
     *    if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_B))
     *        buttons &= ~PAD_CIRCLE;
     *    ... etc.
     *    g_pad.current[port].buttons = buttons;
     *
     * 3. Read analog sticks:
     *    g_pad.current[port].lx = axis_to_ps2(SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_LEFTX));
     *    g_pad.current[port].ly = axis_to_ps2(SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_LEFTY));
     *    g_pad.current[port].rx = axis_to_ps2(SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_RIGHTX));
     *    g_pad.current[port].ry = axis_to_ps2(SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_RIGHTY));
     *
     * SDL controller → PS2 button mapping:
     *   SDL A           → PAD_CROSS
     *   SDL B           → PAD_CIRCLE
     *   SDL X           → PAD_SQUARE
     *   SDL Y           → PAD_TRIANGLE
     *   SDL LEFT_SHOULDER  → PAD_L1
     *   SDL RIGHT_SHOULDER → PAD_R1
     *   SDL TRIGGERLEFT    → PAD_L2  (treat as digital button)
     *   SDL TRIGGERRIGHT   → PAD_R2
     *   SDL BACK        → PAD_SELECT
     *   SDL START       → PAD_START
     *   SDL GUIDE       → (menu)
     *   SDL LEFTSTICK   → PAD_L3
     *   SDL RIGHTSTICK  → PAD_R3
     *   SDL DPAD_UP     → PAD_UP
     *   SDL DPAD_DOWN   → PAD_DOWN
     *   SDL DPAD_LEFT   → PAD_LEFT
     *   SDL DPAD_RIGHT  → PAD_RIGHT
     */
}

const PadData *pad_hal_get_data(u32 port) {
    if (port >= MAX_PORTS) return NULL;
    return &g_pad.current[port];
}

const PadData *pad_hal_get_prev_data(u32 port) {
    if (port >= MAX_PORTS) return NULL;
    return &g_pad.previous[port];
}

#endif /* TARGET_PC */

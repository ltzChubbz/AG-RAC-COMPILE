/**
 * pad.c — PS2 PAD HAL Implementation (stub)
 *
 * Translates SDL2 gamepad events into PS2 DualShock 2 controller format.
 */

#include "pad.h"
#include <stdio.h>
#include <string.h>

#ifdef TARGET_PC

#include <SDL2/SDL.h>

/* ── Internal State ─────────────────────────────────────────────────────────── */

#define MAX_PORTS 2

typedef struct PadHalState {
    PadData current[MAX_PORTS];
    PadData previous[MAX_PORTS];
    SDL_GameController *controllers[MAX_PORTS];
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
        g_pad.controllers[i] = NULL;
        /* Start with all buttons "not pressed" (active-low → all 1s) */
        g_pad.current[i].buttons  = 0xFFFF;
        g_pad.previous[i].buttons = 0xFFFF;
        g_pad.current[i].mode     = 0x7;  /* DualShock2 mode */
        /* Center analog sticks */
        g_pad.current[i].lx = g_pad.current[i].ly = 128;
        g_pad.current[i].rx = g_pad.current[i].ry = 128;
    }
    
    int joystick_count = SDL_NumJoysticks();
    printf("[PAD-HAL] SDL reports %d joystick(s) connected\n", joystick_count);

    int port = 0;
    for (int i = 0; i < joystick_count && port < MAX_PORTS; i++) {
        const char *name = SDL_JoystickNameForIndex(i);
        int is_controller = SDL_IsGameController(i);
        printf("[PAD-HAL]  - Joystick %d: '%s' (is_controller: %s)\n", 
               i, name ? name : "Unknown", is_controller ? "YES" : "NO");

        if (is_controller) {
            g_pad.controllers[port] = SDL_GameControllerOpen(i);
            if (g_pad.controllers[port]) {
                printf("[PAD-HAL]    Mapped to Port %d\n", port + 1);
                port++;
            }
        }
    }

    g_pad.initialized = 1;
    printf("[PAD-HAL] Initialization complete\n");
}

void pad_hal_shutdown(void) {
    if (!g_pad.initialized) return;
    for (int i = 0; i < MAX_PORTS; i++) {
        if (g_pad.controllers[i]) {
            SDL_GameControllerClose(g_pad.controllers[i]);
            g_pad.controllers[i] = NULL;
        }
    }
    memset(&g_pad, 0, sizeof(g_pad));
}

void pad_hal_update(void) {
    if (!g_pad.initialized) return;

    for (int i = 0; i < MAX_PORTS; i++) {
        SDL_GameController *ctrl = g_pad.controllers[i];
        
        /* Save previous state */
        memcpy(&g_pad.previous[i], &g_pad.current[i], sizeof(PadData));
        
        if (!ctrl || !SDL_GameControllerGetAttached(ctrl)) {
            /* Keep centered/unpressed if disconnected */
            g_pad.current[i].buttons = 0xFFFF;
            g_pad.current[i].lx = g_pad.current[i].ly = 128;
            g_pad.current[i].rx = g_pad.current[i].ry = 128;
            continue;
        }

        /* ── Buttons (PS2: Active-Low) ── */
        u16 btn = 0xFFFF;
        
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_UP))    btn &= ~PAD_UP;
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  btn &= ~PAD_DOWN;
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  btn &= ~PAD_LEFT;
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) btn &= ~PAD_RIGHT;
        
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_A))          btn &= ~PAD_CROSS;
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_B))          btn &= ~PAD_CIRCLE;
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_X))          btn &= ~PAD_SQUARE;
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_Y))          btn &= ~PAD_TRIANGLE;
        
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_LEFTSHOULDER))  btn &= ~PAD_L1;
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) btn &= ~PAD_R1;
        
        /* Triggers (Digital map) */
        if (SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16000)  btn &= ~PAD_L2;
        if (SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16000) btn &= ~PAD_R2;
        
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_START))  btn &= ~PAD_START;
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_BACK))   btn &= ~PAD_SELECT;
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_LEFTSTICK))  btn &= ~PAD_L3;
        if (SDL_GameControllerGetButton(ctrl, SDL_CONTROLLER_BUTTON_RIGHTSTICK)) btn &= ~PAD_R3;

        g_pad.current[i].buttons = btn;

        /* ── Analog Sticks ── */
        g_pad.current[i].lx = axis_to_ps2(SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_LEFTX));
        g_pad.current[i].ly = axis_to_ps2(SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_LEFTY));
        g_pad.current[i].rx = axis_to_ps2(SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_RIGHTX));
        g_pad.current[i].ry = axis_to_ps2(SDL_GameControllerGetAxis(ctrl, SDL_CONTROLLER_AXIS_RIGHTY));
    }
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

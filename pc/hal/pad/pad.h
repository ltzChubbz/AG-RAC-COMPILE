/**
 * pad.h — PS2 Controller Input (SIO2/PADMAN) HAL
 *
 * On PS2, controller input is managed by the IOP via SIO2MAN and PADMAN
 * IRX modules. The game reads a 32-bit button bitmask and two pairs of
 * analog stick bytes via a shared memory struct updated by the IOP.
 *
 * On PC, we translate SDL2 gamepad input into the PS2 controller format,
 * so decompiled game code can read input without modification.
 */

#ifndef AG_RAC_HAL_PAD_H
#define AG_RAC_HAL_PAD_H

#include "../../include/types.h"

/* ── PS2 Button Bitmask (DUALSHOCK 2) ─────────────────────────────────────── */
/* Bit = 0 means PRESSED (active-low, matching PS2 hardware) */

#define PAD_SELECT    (1 << 0)
#define PAD_L3        (1 << 1)
#define PAD_R3        (1 << 2)
#define PAD_START     (1 << 3)
#define PAD_UP        (1 << 4)
#define PAD_RIGHT     (1 << 5)
#define PAD_DOWN      (1 << 6)
#define PAD_LEFT      (1 << 7)
#define PAD_L2        (1 << 8)
#define PAD_R2        (1 << 9)
#define PAD_L1        (1 << 10)
#define PAD_R1        (1 << 11)
#define PAD_TRIANGLE  (1 << 12)
#define PAD_CIRCLE    (1 << 13)
#define PAD_CROSS     (1 << 14)
#define PAD_SQUARE    (1 << 15)

/* ── PS2 Pad State Struct ──────────────────────────────────────────────────── */
/* Matches the layout the game reads from shared IOP memory */

typedef struct PadData {
    u8  mode;        /* Controller mode byte (0x7 = DualShock2) */
    u8  status;      /* Status byte */
    u16 buttons;     /* Button bitmask (active-low) */
    u8  rx;          /* Right stick X (128 = center) */
    u8  ry;          /* Right stick Y (128 = center) */
    u8  lx;          /* Left stick X  (128 = center) */
    u8  ly;          /* Left stick Y  (128 = center) */
    /* Pressure-sensitive button values (DualShock2 only) */
    u8  right_p;
    u8  left_p;
    u8  up_p;
    u8  down_p;
    u8  triangle_p;
    u8  circle_p;
    u8  cross_p;
    u8  square_p;
    u8  l1_p;
    u8  r1_p;
    u8  l2_p;
    u8  r2_p;
} PadData;

/* ── Helper Macros ─────────────────────────────────────────────────────────── */

/* Check if a button is currently pressed (note: active-low!) */
#define PAD_PRESSED(pad, btn)  (!((pad).buttons & (btn)))

/* Check if pressed this frame but not last frame (rising edge) */
#define PAD_JUST_PRESSED(curr, prev, btn) \
    (PAD_PRESSED(curr, btn) && !PAD_PRESSED(prev, btn))

/* ── HAL Public API ────────────────────────────────────────────────────────── */

/**
 * pad_hal_init() — Initialize the PAD HAL (SDL2 gamepad subsystem).
 */
void pad_hal_init(void);

/**
 * pad_hal_shutdown() — Shutdown the PAD HAL.
 */
void pad_hal_shutdown(void);

/**
 * pad_hal_update() — Poll and update controller state.
 *   Must be called once per frame before the game reads pad data.
 */
void pad_hal_update(void);

/**
 * pad_hal_get_data() — Get the current pad state for a given port.
 *   @param port   0 = port 1 (player 1), 1 = port 2 (player 2)
 *   @return       Pointer to the current PadData, or NULL if disconnected
 */
const PadData *pad_hal_get_data(u32 port);

/**
 * pad_hal_get_prev_data() — Get pad state from the previous frame.
 *   Used for edge detection (just-pressed, just-released).
 */
const PadData *pad_hal_get_prev_data(u32 port);

#endif /* AG_RAC_HAL_PAD_H */

/**
 * dma.h — PS2 DMA Controller HAL
 *
 * On PS2, DMA transfers move data between RAM and hardware peripherals
 * (GS, VU0/1, SPU2, IOP) without CPU involvement.
 *
 * On PC, all DMA is synchronous — "transfers" are just immediate function calls.
 * This header provides wrappers so decompiled code that initiates DMA
 * compiles cleanly on PC.
 */

#ifndef AG_RAC_HAL_DMA_H
#define AG_RAC_HAL_DMA_H

#include "../../include/types.h"

/* PS2 DMA Channel IDs */
#define DMA_CH_VIF0   0   /* VIF0 → VU0 */
#define DMA_CH_VIF1   1   /* VIF1 → VU1 */
#define DMA_CH_GIF    2   /* GIF → GS */
#define DMA_CH_SPR0   8   /* Main RAM → Scratchpad */
#define DMA_CH_SPR1   9   /* Scratchpad → Main RAM */

/**
 * dma_send() — Initiate a DMA transfer.
 *   On PC this is a no-op wrapper — data is already where it needs to be.
 *   The relevant HAL (GS/VU) will be called directly by the game code.
 */
static inline void dma_send(u32 channel, const void *data, u32 qwc) {
    /* qwc = quadword count (1 qw = 16 bytes) */
    (void)channel; (void)data; (void)qwc;
    /* No-op on PC — data flow handled by direct HAL calls */
}

/**
 * dma_wait() — Wait for a DMA channel to complete.
 *   On PC this is always instant — returns immediately.
 */
static inline void dma_wait(u32 channel) {
    (void)channel;
}

/**
 * dma_send_chain() — Send a DMA source chain (linked list of tags).
 *   On PC: iterate the chain and call the appropriate handler per tag.
 */
void dma_send_chain(u32 channel, const void *tag_addr);

#endif /* AG_RAC_HAL_DMA_H */

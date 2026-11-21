// DMA engine for the HUB75 scan (XMC only): GPDMA0 channel 0 copies one
// encoded bit-plane (see hub75_encode.h) to PORT5->OMR, one 32-bit write per
// request. A CCU42 timer issues the requests at a fixed rate through DMA
// request line 0, so the shift clock runs at write_hz / 2 with no CPU work.
// The block-complete interrupt is GPDMA0_0_IRQHandler, in hub75.c.

#ifndef HUB75_DMA_H
#define HUB75_DMA_H

#include <stdint.h>

// set up the channel and start the pacing timer (no transfer yet)
void hub75_dma_init(uint32_t write_hz);

// copy `count` words from `words` to PORT5->OMR; completes with an interrupt
void hub75_dma_start(const uint32_t *words, uint32_t count);

// call from the interrupt handler: acknowledge the block-complete event
void hub75_dma_acknowledge(void);

#endif // HUB75_DMA_H

// Turns framebuffer rows into the port writes that shift them into the
// panels, one sequence per bit-plane (binary code modulation). Plain C with no
// hardware access, so it builds and is tested on a laptop (sim/test_encode.c).
//
// For row pair r (rows r and r+32) and plane p, the sequence is two 32-bit
// writes to PORT5->OMR per chain column:
//   [2x]   RGB bits of plane p (R1 G1 B1 R2 G2 B2 on P5.0-5) with CLK low
//   [2x+1] CLK high (the panel samples on this rising edge)
// OMR: the low 16 bits set pins, the high 16 clear them; only the pins named
// change, so LAT, OE and the kit's LEDs on P5 are untouched.

#ifndef HUB75_ENCODE_H
#define HUB75_ENCODE_H

#include <stdint.h>
#include "hub75_fb.h"

#define HUB75_PLANES    4                      // 4 bits per channel (RGB444)
#define HUB75_ROW_PAIRS (HUB75_HEIGHT / 2U)
#define HUB75_ROW_WORDS (HUB75_CHAIN_WIDTH * 2U) // OMR writes per plane

// P5 pin masks
#define HUB75_RGB_MASK   UINT32_C(0x003f) // P5.0-5: R1 G1 B1 R2 G2 B2
#define HUB75_CLOCK_MASK (UINT32_C(1) << 6)
#define HUB75_LATCH_MASK (UINT32_C(1) << 7)
#define HUB75_OE_MASK    (UINT32_C(1) << 10)

typedef uint32_t hub75_row_planes_t[HUB75_PLANES][HUB75_ROW_WORDS];

// Build the lookup table and fill in the constant CLK-high words of `rows`.
void hub75_encode_init(hub75_row_planes_t *rows, uint32_t count);

// Encode row pair `row` (0..31) of `frame` into `out` (the data words only;
// the CLK-high words were written by hub75_encode_init).
void hub75_encode_row(const hub75_frame_t *frame, uint32_t row, hub75_row_planes_t *out);

#endif // HUB75_ENCODE_H

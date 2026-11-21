#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O2") // runs once per row, 3840 times a second
#endif

#include "hub75_encode.h"

_Static_assert(HUB75_PLANES == 4, "hub75_encode_row is unrolled for 4 planes");

// For each RGB444 color: its RGB bits for each plane, one byte per plane
// (byte p = bit p of red | green << 1 | blue << 2). A pixel pair is then
// spread[top] | spread[bottom] << 3: byte p holds plane p's 6 data bits.
static uint32_t spread[4096];

// 6 data bits -> the finished OMR write (data set, others + CLK cleared)
static uint32_t omr_word[64];

void hub75_encode_init(hub75_row_planes_t *rows, uint32_t count) {
    for (uint32_t c = 0; c < 4096; ++c) {
        const uint32_t r = (c >> 8) & 0xF, g = (c >> 4) & 0xF, b = c & 0xF;
        uint32_t packed = 0;
        for (uint32_t p = 0; p < HUB75_PLANES; ++p) {
            const uint32_t bits = ((r >> p) & 1U) | (((g >> p) & 1U) << 1) | (((b >> p) & 1U) << 2);
            packed |= bits << (8U * p);
        }
        spread[c] = packed;
    }
    for (uint32_t d = 0; d < 64; ++d) {
        omr_word[d] = d | (((~d & HUB75_RGB_MASK) | HUB75_CLOCK_MASK) << 16);
    }
    for (uint32_t i = 0; i < count; ++i) {
        for (uint32_t p = 0; p < HUB75_PLANES; ++p) {
            for (uint32_t x = 0; x < HUB75_CHAIN_WIDTH; ++x) {
                rows[i][p][2 * x] = omr_word[0]; // data written per row
                rows[i][p][2 * x + 1] = HUB75_CLOCK_MASK;
            }
        }
    }
}

void hub75_encode_row(const hub75_frame_t *frame, uint32_t row, hub75_row_planes_t *out) {
    const hub75_color_t *upper = (*frame)[row];
    const hub75_color_t *lower = (*frame)[row + HUB75_ROW_PAIRS];
    uint32_t *p0 = (*out)[0], *p1 = (*out)[1], *p2 = (*out)[2], *p3 = (*out)[3];
    for (uint32_t x = 0; x < HUB75_CHAIN_WIDTH; ++x) {
        const uint32_t d = spread[upper[x] & 0x0FFFU] | (spread[lower[x] & 0x0FFFU] << 3);
        p0[2 * x] = omr_word[d & 0x3FU];
        p1[2 * x] = omr_word[(d >> 8) & 0x3FU];
        p2[2 * x] = omr_word[(d >> 16) & 0x3FU];
        p3[2 * x] = omr_word[(d >> 24) & 0x3FU];
    }
}

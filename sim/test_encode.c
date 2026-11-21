// Laptop test for base/src/display/hub75_encode.c:  make test
#include <stdio.h>
#include <string.h>
#include "hub75_encode.h"

static hub75_frame_t frame;
static hub75_row_planes_t rows[1];
static int failures;

static void expect(const char *what, uint32_t got, uint32_t want) {
    if (got != want) {
        printf("FAIL %s: got 0x%08x want 0x%08x\n", what, (unsigned)got, (unsigned)want);
        failures++;
    }
}

static uint32_t word(uint32_t data) { // OMR write: data bits set, others + CLK cleared
    return data | (((~data & HUB75_RGB_MASK) | HUB75_CLOCK_MASK) << 16);
}

int main(void) {
    hub75_encode_init(rows, 1);

    // top pixel RGB(5,10,15), bottom pixel (row + 32) RGB(1,0,8), column 3
    memset(frame, 0, sizeof frame);
    frame[2][3] = HUB75_RGB(5, 10, 15);
    frame[2 + HUB75_ROW_PAIRS][3] = HUB75_RGB(1, 0, 8);
    hub75_encode_row(&frame, 2, &rows[0]);

    // plane p data bits: R1 G1 B1 R2 G2 B2 = bit p of r,g,b (top) and (bottom)
    const uint32_t want[HUB75_PLANES] = {
        0x05 | 0x08,       // plane 0: R1 B1 | R2
        0x06,              // plane 1: G1 B1
        0x05,              // plane 2: R1 B1
        0x06 | 0x20,       // plane 3: G1 B1 | B2
    };
    char what[64];
    for (uint32_t p = 0; p < HUB75_PLANES; ++p) {
        snprintf(what, sizeof what, "plane %u column 3 data", (unsigned)p);
        expect(what, rows[0][p][2 * 3], word(want[p]));
        snprintf(what, sizeof what, "plane %u column 3 clock", (unsigned)p);
        expect(what, rows[0][p][2 * 3 + 1], HUB75_CLOCK_MASK);
        snprintf(what, sizeof what, "plane %u black column", (unsigned)p);
        expect(what, rows[0][p][2 * 7], word(0));
    }
    // every CLK-high word is just CLK, every data word keeps CLK low
    for (uint32_t p = 0; p < HUB75_PLANES; ++p) {
        for (uint32_t x = 0; x < HUB75_CHAIN_WIDTH; ++x) {
            if (rows[0][p][2 * x + 1] != HUB75_CLOCK_MASK ||
                !(rows[0][p][2 * x] & (HUB75_CLOCK_MASK << 16))) {
                printf("FAIL plane %u column %u clock words\n", (unsigned)p, (unsigned)x);
                failures++;
            }
        }
    }
    // full white lights every data line in every plane
    for (uint32_t x = 0; x < HUB75_CHAIN_WIDTH; ++x) {
        frame[5][x] = frame[5 + HUB75_ROW_PAIRS][x] = HUB75_WHITE;
    }
    hub75_encode_row(&frame, 5, &rows[0]);
    for (uint32_t p = 0; p < HUB75_PLANES; ++p) {
        snprintf(what, sizeof what, "white plane %u", (unsigned)p);
        expect(what, rows[0][p][0], word(HUB75_RGB_MASK));
    }

    printf("%s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

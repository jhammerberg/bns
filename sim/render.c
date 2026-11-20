// Draw the virtual panels: live in the terminal, and as PNG snapshots.

#include <stdio.h>
#include <string.h>
#include "sim.h"

#undef printf // sim output goes to the real stdout, not the UART log

static void led_rgb(uint8_t bits, bool lit, uint8_t rgb[3], uint8_t off) {
    if (!lit || !bits) {
        rgb[0] = rgb[1] = rgb[2] = off; // unlit LED
        return;
    }
    rgb[0] = (bits & 1) ? 255 : 0;
    rgb[1] = (bits & 2) ? 255 : 0;
    rgb[2] = (bits & 4) ? 255 : 0;
}

// ---- terminal: two LEDs per character cell (upper-half block) ----

void render_terminal(const sim_glass_t *g, const char *status, const char *uart_tail) {
    fputs("\x1b[H", stdout); // cursor home; redraw in place
    for (uint32_t y = 0; y < HUB75_HEIGHT; y += 2) {
        for (uint32_t x = 0; x < HUB75_CHAIN_WIDTH; ++x) {
            if (x != 0 && x % HUB75_WIDTH == 0) {
                fputs("\x1b[0m ", stdout); // gap between panels
            }
            uint8_t top[3], bot[3];
            led_rgb(g->px[y][x], g->lit, top, 40);
            led_rgb(g->px[y + 1][x], g->lit, bot, 40);
            printf("\x1b[38;2;%u;%u;%um\x1b[48;2;%u;%u;%um\xe2\x96\x80",
                   top[0], top[1], top[2], bot[0], bot[1], bot[2]);
        }
        fputs("\x1b[0m\x1b[K\n", stdout);
    }
    printf("\x1b[K%s\n\x1b[K-- uart --\n", status);
    // print the UART tail, clearing each line so old text does not linger
    for (const char *p = uart_tail; *p; ++p) {
        if (*p == '\n') fputs("\x1b[K", stdout);
        fputc(*p, stdout);
    }
    fputs("\x1b[K\x1b[J", stdout);
    fflush(stdout);
}

// ---- PNG: one 8x8 square per LED, written with stb_image_write ----
// stb_image_write.h: github.com/nothings/stb @ 6b5799b (v1.19, public domain)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define LED_PX 8 // image pixels per LED; the last row/column is a grid line

static uint8_t img[HUB75_HEIGHT * LED_PX][HUB75_CHAIN_WIDTH * LED_PX][3];

int render_png(const char *path, const sim_glass_t *g) {
    for (uint32_t py = 0; py < HUB75_HEIGHT * LED_PX; ++py) {
        for (uint32_t px = 0; px < HUB75_CHAIN_WIDTH * LED_PX; ++px) {
            uint8_t *out = img[py][px];
            const bool grid = px % LED_PX == LED_PX - 1 || py % LED_PX == LED_PX - 1;
            const bool panel_edge = px % (HUB75_WIDTH * LED_PX) == 0 && px != 0;
            if (panel_edge) {
                out[0] = out[1] = out[2] = 120; // line between chained panels
            } else if (grid) {
                out[0] = out[1] = out[2] = 12;
            } else {
                led_rgb(g->px[py / LED_PX][px / LED_PX], g->lit, out, 40);
            }
        }
    }

    // write to a temp file and rename, so a reader never sees half a PNG
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    const int w = HUB75_CHAIN_WIDTH * LED_PX, h = HUB75_HEIGHT * LED_PX;
    if (!stbi_write_png(tmp, w, h, 3, img, w * 3)) {
        return -1;
    }
    return rename(tmp, path);
}

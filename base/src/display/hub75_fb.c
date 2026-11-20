// Drawing into the framebuffer. No hardware access here, so this file also
// builds on a laptop for the simulator (sim/).

#include "hub75_fb.h"

uint8_t hub75_framebuffer[HUB75_HEIGHT][HUB75_CHAIN_WIDTH];

// column where the selected panel starts in the chain
static uint32_t panel_x0;

static uint8_t color_bits(hub75_color_t color) {
    return (uint8_t)((uint32_t)color & UINT32_C(7));
}

void hub75_fb_reset(void) {
    panel_x0 = 0;
    for (uint32_t y = 0; y < HUB75_HEIGHT; ++y) {
        for (uint32_t x = 0; x < HUB75_CHAIN_WIDTH; ++x) {
            hub75_framebuffer[y][x] = 0;
        }
    }
}

void hub75_select_panel(uint32_t panel) {
    if (panel < HUB75_PANELS) {
        panel_x0 = panel * HUB75_WIDTH;
    }
}

void hub75_clear(void) {
    hub75_fill(HUB75_BLACK);
}

void hub75_fill(hub75_color_t color) {
    const uint8_t value = color_bits(color);
    for (uint32_t y = 0; y < HUB75_HEIGHT; ++y) {
        for (uint32_t x = 0; x < HUB75_WIDTH; ++x) {
            hub75_framebuffer[y][panel_x0 + x] = value;
        }
    }
}

void hub75_set_pixel(int32_t x, int32_t y, hub75_color_t color) {
    if (x < 0 || y < 0 || (uint32_t)x >= HUB75_WIDTH || (uint32_t)y >= HUB75_HEIGHT) {
        return;
    }
    hub75_framebuffer[y][panel_x0 + (uint32_t)x] = color_bits(color);
}

hub75_color_t hub75_get_pixel(int32_t x, int32_t y) {
    if (x < 0 || y < 0 || (uint32_t)x >= HUB75_WIDTH || (uint32_t)y >= HUB75_HEIGHT) {
        return HUB75_BLACK;
    }
    return (hub75_color_t)hub75_framebuffer[y][panel_x0 + (uint32_t)x];
}

void hub75_fill_rect(int32_t x, int32_t y, int32_t width, int32_t height, hub75_color_t color) {
    if (width <= 0 || height <= 0) {
        return;
    }

    // use 64 bits here so large coordinates do not overflow before clipping
    int64_t left = x;
    int64_t top = y;
    int64_t right = left + (int64_t)width;
    int64_t bottom = top + (int64_t)height;

    if (right <= 0 || bottom <= 0 || left >= HUB75_WIDTH || top >= HUB75_HEIGHT) {
        return;
    }
    if (left < 0) {
        left = 0;
    }
    if (top < 0) {
        top = 0;
    }
    if (right > HUB75_WIDTH) {
        right = HUB75_WIDTH;
    }
    if (bottom > HUB75_HEIGHT) {
        bottom = HUB75_HEIGHT;
    }

    const uint8_t value = color_bits(color);
    for (uint32_t py = (uint32_t)top; py < (uint32_t)bottom; ++py) {
        for (uint32_t px = (uint32_t)left; px < (uint32_t)right; ++px) {
            hub75_framebuffer[py][panel_x0 + px] = value;
        }
    }
}

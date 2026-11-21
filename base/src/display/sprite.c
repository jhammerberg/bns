#include "sprite.h"
#include <stdbool.h>
#include <string.h>

// colour letter -> panel colour; returns false for transparent pixels
static bool pixel_color(char c, hub75_color_t *out) {
    switch (c) {
    case 'R': *out = HUB75_RED; return true;
    case 'G': *out = HUB75_GREEN; return true;
    case 'B': *out = HUB75_BLUE; return true;
    case 'Y': *out = HUB75_YELLOW; return true;
    case 'C': *out = HUB75_CYAN; return true;
    case 'M': *out = HUB75_MAGENTA; return true;
    case 'W': *out = HUB75_WHITE; return true;
    case ' ': return false;
    default:  *out = HUB75_BLACK; return true; // '.' and anything unknown
    }
}

static void draw(const sprite_t *sprite, int32_t x, int32_t y, int scale, bool tinted,
                 hub75_color_t tint) {
    if (sprite == NULL) {
        return;
    }
    for (int row = 0; row < SPRITE_MAX_ROWS && sprite->rows[row] != NULL; ++row) {
        const char *pixels = sprite->rows[row];
        for (int col = 0; pixels[col] != '\0'; ++col) {
            hub75_color_t color;
            if (!pixel_color(pixels[col], &color)) {
                continue;
            }
            if (tinted && color != HUB75_BLACK) {
                color = tint;
            }
            hub75_fill_rect(x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

void sprite_draw(const sprite_t *sprite, int32_t x, int32_t y) {
    draw(sprite, x, y, 1, false, HUB75_BLACK);
}

void sprite_draw_tinted(const sprite_t *sprite, int32_t x, int32_t y, hub75_color_t color) {
    draw(sprite, x, y, 1, true, color);
}

void sprite_draw_scaled(const sprite_t *sprite, int32_t x, int32_t y, int scale) {
    draw(sprite, x, y, scale, false, HUB75_BLACK);
}

void sprite_draw_scaled_tinted(const sprite_t *sprite, int32_t x, int32_t y, int scale,
                               hub75_color_t color) {
    draw(sprite, x, y, scale, true, color);
}

int sprite_width(const sprite_t *sprite) {
    return sprite->rows[0] ? (int)strlen(sprite->rows[0]) : 0;
}

int sprite_height(const sprite_t *sprite) {
    int rows = 0;
    while (rows < SPRITE_MAX_ROWS && sprite->rows[rows] != NULL) {
        ++rows;
    }
    return rows;
}

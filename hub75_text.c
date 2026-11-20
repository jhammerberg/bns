// draw characters using the GFX font
// each byte is one column, with bit 0 at the top

#include "hub75_text.h"
#include "gfx_font.h" // just the font table

void hub75_draw_char(char character, int32_t x, int32_t y, hub75_color_t foreground, hub75_color_t background) {
    // skip boxes that are completely outside the screen
    if (x <= -(int32_t)HUB75_CHAR_WIDTH ||
        y <= -(int32_t)HUB75_CHAR_HEIGHT ||
        x >= (int32_t)HUB75_WIDTH || y >= (int32_t)HUB75_HEIGHT) {
        return;
    }

    // use ? if this is not a printable ASCII character
    uint8_t code = (uint8_t)(unsigned char)character;
    if (code < 0x20U || code > 0x7EU) {
        code = (uint8_t)'?';
    }

    for (uint32_t col = 0; col < HUB75_CHAR_WIDTH; ++col) {
        // first five columns are the glyph, last column is background spacing
        const uint8_t bits = col < HUB75_GLYPH_WIDTH
            ? font[(uint32_t)code * HUB75_GLYPH_WIDTH + col] : 0;

        for (uint32_t row = 0; row < HUB75_CHAR_HEIGHT; ++row) {
            const hub75_color_t color = (bits & (UINT32_C(1) << row))
                ? foreground : background;
            hub75_set_pixel(x + (int32_t)col, y + (int32_t)row, color);
        }
    }
}

void hub75_draw_text(const char *text, int32_t x, int32_t y, hub75_color_t foreground, hub75_color_t background) {
    while (*text) {
        hub75_draw_char(*text++, x, y, foreground, background);
        x += HUB75_CHAR_WIDTH;
    }
}

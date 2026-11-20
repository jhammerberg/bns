// draw characters using the GFX font
// each byte is one column, with bit 0 at the top

#include <stdbool.h>
#include "hub75_text.h"
#include "gfx_font.h" // just the font table
#include "font_small.h"

void hub75_draw_char(char character, int32_t x, int32_t y, hub75_color_t foreground, hub75_color_t background) {
    // skip boxes that are completely outside the screen
    if (x <= -(int32_t)HUB75_CHAR_WIDTH ||
        y <= -(int32_t)HUB75_CHAR_HEIGHT ||
        x >= (int32_t)HUB75_WIDTH || y >= (int32_t)HUB75_HEIGHT) {
        return;
    }

    // use ? if this is not printable ASCII or one of the arrow glyphs (0x18..0x1B)
    uint8_t code = (uint8_t)(unsigned char)character;
    const bool is_arrow = code >= 0x18U && code <= 0x1BU;
    if (!is_arrow && (code < 0x20U || code > 0x7EU)) {
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

void hub75_draw_char_small(char character, int32_t x, int32_t y, hub75_color_t foreground, hub75_color_t background) {
    if (x <= -(int32_t)HUB75_SMALL_CHAR_WIDTH ||
        y <= -(int32_t)HUB75_SMALL_CHAR_HEIGHT ||
        x >= (int32_t)HUB75_WIDTH || y >= (int32_t)HUB75_HEIGHT) {
        return;
    }

    uint8_t code = (uint8_t)(unsigned char)character;
    if (code >= 'a' && code <= 'z') {
        code = (uint8_t)(code - 'a' + 'A');
    }

    // find the glyph: ASCII 0x20..0x5F first, then the four arrows
    uint32_t glyph;
    if (code >= FONT_SMALL_FIRST && code <= FONT_SMALL_LAST) {
        glyph = code - FONT_SMALL_FIRST;
    } else if (code >= FONT_SMALL_ARROW_FIRST && code <= FONT_SMALL_ARROW_LAST) {
        glyph = FONT_SMALL_ARROW_INDEX + (code - FONT_SMALL_ARROW_FIRST);
    } else {
        glyph = (uint32_t)('?' - FONT_SMALL_FIRST);
    }
    const unsigned char *rows = &font_small[glyph * HUB75_SMALL_GLYPH_HEIGHT];

    for (uint32_t row = 0; row < HUB75_SMALL_CHAR_HEIGHT; ++row) {
        // last row is background spacing
        const uint8_t bits = row < HUB75_SMALL_GLYPH_HEIGHT ? rows[row] : 0;
        for (uint32_t col = 0; col < HUB75_SMALL_CHAR_WIDTH; ++col) {
            // bit 2 is the left column; the fourth column is spacing
            const bool lit = col < HUB75_SMALL_GLYPH_WIDTH &&
                (bits & (1U << (HUB75_SMALL_GLYPH_WIDTH - 1U - col)));
            hub75_set_pixel(x + (int32_t)col, y + (int32_t)row, lit ? foreground : background);
        }
    }
}

void hub75_draw_text_small(const char *text, int32_t x, int32_t y, hub75_color_t foreground, hub75_color_t background) {
    while (*text) {
        hub75_draw_char_small(*text++, x, y, foreground, background);
        x += HUB75_SMALL_CHAR_WIDTH;
    }
}

/*
 * After init, draw a character like this:
 *
 *     hub75_clear();
 *     hub75_draw_char('A', 10, 20, HUB75_WHITE, HUB75_BLACK);
 *
 * Keep calling hub75_refresh() in main to show it.
 * Use single quotes for a character, like 'A'.
 *
 * (x,y) is the top-left of the 6x8 box, not the first lit pixel.
 * The font takes five columns and up to eight rows. The sixth column is
 * spacing. Some lowercase letters use the bottom row for their tails.
 * Move x by 6 for the next character and y by 8 for the next line.
 *
 *     hub75_draw_char('H', 0, 0, HUB75_GREEN, HUB75_BLACK);
 *     hub75_draw_char('P', 6, 0, HUB75_GREEN, HUB75_BLACK);
 *     hub75_draw_char(':', 12, 0, HUB75_WHITE, HUB75_BLACK);
 *     hub75_draw_char('9', 18, 0, HUB75_RED, HUB75_BLACK);
 *
 * This writes the whole box, including background and spacing, so a new
 * character erases the old one. A space clears the box to the background.
 * It also covers any picture underneath, so pick the background accordingly.
 *
 * Characters can start off-screen; only the visible part gets drawn.
 * There is no automatic wrapping. Printable ASCII (0x20..0x7E) works;
 * anything else, including newline, draws a question mark. No UTF-8 or scaling.
 *
 * Draw between refresh calls, not from an interrupt while scanning.
 */

#ifndef HUB75_TEXT_H
#define HUB75_TEXT_H

#include "hub75.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HUB75_GLYPH_WIDTH  5U
#define HUB75_GLYPH_HEIGHT 8U
#define HUB75_CHAR_WIDTH   6U
#define HUB75_CHAR_HEIGHT  8U

// draw the character at (x,y), filling unused pixels with background
// anything outside the screen gets clipped by set_pixel
void hub75_draw_char(char character, int32_t x, int32_t y,
                     hub75_color_t foreground, hub75_color_t background);

#ifdef __cplusplus
}
#endif

void hub75_draw_text(const char *text, int32_t x, int32_t y,
                     hub75_color_t foreground, hub75_color_t background);

#endif // HUB75_TEXT_H

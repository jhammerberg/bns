/*
 * Driver for the chained 64x64 HUB75 panels.
 *
 * hub75_init() starts a timer interrupt that scans one row pair at a time in
 * the background, so the panels stay lit and evenly bright whatever the game
 * loop is doing. Drawing goes into a back buffer; hub75_present() hands it to
 * the display at the next frame boundary (double buffering, no half-drawn
 * frames) and waits for that, which also paces the game loop to the frame
 * rate. After present the back buffer holds an old frame: redraw everything.
 *
 * x and y are 0..63 within the selected panel. Pixels outside are ignored,
 * rectangles are clipped. Colors use bits 0, 1, 2 for red, green, blue.
 * Pass the CPU clock in Hz; delays use the DWT cycle counter.
 */

#ifndef HUB75_H
#define HUB75_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Size of ONE panel. Drawing coordinates are always 0..63 within the
// selected panel (see hub75_select_panel).
#define HUB75_WIDTH  64U
#define HUB75_HEIGHT 64U

// Panels daisy-chained on the one HUB75 output. Panel 0 is the one wired to
// the XMC, panel 1 the next one along the chain (checked on hardware).
// base/Makefile sets -DHUB75_PANELS=2.
#ifndef HUB75_PANELS
#define HUB75_PANELS 1U
#endif
#define HUB75_CHAIN_WIDTH (HUB75_WIDTH * HUB75_PANELS)

typedef enum {
    HUB75_BLACK   = 0,
    HUB75_RED     = 1,
    HUB75_GREEN   = 2,
    HUB75_YELLOW  = 3,
    HUB75_BLUE    = 4,
    HUB75_MAGENTA = 5,
    HUB75_CYAN    = 6,
    HUB75_WHITE   = 7
} hub75_color_t;

typedef enum {
    HUB75_OK = 0,
    HUB75_ERROR_CLOCK,   // cpu_hz is outside 1..144 MHz
    HUB75_ERROR_COUNTER  // DWT counter did not advance
} hub75_status_t;

// set up the pins/counter and start with a black image
// call after board init. On an error, OE stays high
// calling this again clears anything already drawn
hub75_status_t hub75_init(uint32_t cpu_hz);

// choose which chained panel the drawing calls below affect (default 0)
// out-of-range panel numbers are ignored
void hub75_select_panel(uint32_t panel);

// clear the selected panel to black
void hub75_clear(void);

// fill the selected panel with one color
void hub75_fill(hub75_color_t color);

// set one pixel, ignoring coordinates outside the screen
void hub75_set_pixel(int32_t x, int32_t y, hub75_color_t color);

// fill a rectangle from (x,y), clipping any part outside the screen
// x/y can be negative. Width or height <= 0 does nothing
void hub75_fill_rect(int32_t x, int32_t y, int32_t width, int32_t height, hub75_color_t color);

// show what has been drawn: swaps buffers at the next frame boundary and
// waits for it (at most one frame). Does nothing before a successful init
void hub75_present(void);

// turn the LEDs off (true) or back on (false); the image is kept
void hub75_blank(bool blank);

// counters for diagnostics: frames shown, and microseconds spent in the
// display interrupt, both since init
void hub75_get_stats(uint32_t *frames, uint32_t *busy_us);

#ifdef __cplusplus
}
#endif

#endif // HUB75_H

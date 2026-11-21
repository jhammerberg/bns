/*
 * Init after cybsp_init() and SystemCoreClockUpdate(), then draw the image.
 * Keep calling hub75_refresh() in main, even if nothing has changed.
 * Drawing only changes the buffer; refresh actually sends it to the panel.
 * This still uses GPIO writes and busy waits, not DMA or a timer ISR.
 *
 * Pinout is in dev folder.
 *
 * x and y are 0..63. RGB1 handles row n, RGB2 handles row n+32.
 * Columns are sent in increasing x order. If the image looks mirrored,
 * check a drawing that is different on each side before changing the mapping.
 * Pixels outside the screen are ignored. Rectangles are clipped to fit.
 * Colors use bits 0, 1, 2 for red, green, blue. Other bits are ignored.
 *
 * Clear the old area before drawing a shorter bar, or the extra pixels stay.
 * Make drawing changes between refresh calls. There is only one buffer,
 * so do not change it from an interrupt while a scan is reading it.
 *
 * Each call scans all 32 row pairs. The delays take about 4.9 ms, plus the
 * time spent running the code and any interrupts. The LEDs are off between
 * calls, so long waits in main make the display dim or flicker.
 * Moving the same scan into an ISR would still take up the CPU.
 *
 * Pass the CPU clock in Hz and keep it fixed after init. Delays use the DWT
 * cycle counter, so leave that running. This does not need CCU4 or SysTick.
 *
 * Blank just sets OE high; the image stays in RAM. It does not disconnect
 * the signal pins, so the normal power sequence still matters.
 * Pausing with OE low leaves a row on. Blank before a long debugger pause.
 */

#ifndef HUB75_H
#define HUB75_H

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

// clear the selected panel to black, then keep refreshing
void hub75_clear(void);

// fill the selected panel with one color. Still need to refresh afterward
void hub75_fill(hub75_color_t color);

// set one pixel, ignoring coordinates outside the screen
void hub75_set_pixel(int32_t x, int32_t y, hub75_color_t color);

// read the stored color. Returns black if the pixel is off-screen
hub75_color_t hub75_get_pixel(int32_t x, int32_t y);

// fill a rectangle from (x,y), clipping any part outside the screen
// x/y can be negative. Width or height <= 0 does nothing
void hub75_fill_rect(int32_t x, int32_t y, int32_t width, int32_t height, hub75_color_t color);

// scan the whole image once, then return with the LEDs off
// call repeatedly from main. Does nothing before a successful init
void hub75_refresh(void);

// turn the LEDs off without clearing the buffer
// next refresh shows it again. Does nothing before init
void hub75_blank(void);

#ifdef __cplusplus
}
#endif

#endif // HUB75_H

/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 hub75-driver contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * HUB75 DRIVER -- XMC4700 RELAX KIT / MODUSTOOLBOX / ONE 64 x 64 PANEL
 * ==================================================================
 *
 * NOTES
 * ---------------------
 * Stores a 64 x 64 image in a private 4096-byte framebuffer. Each pixel has
 * one red bit, one green bit, and one blue bit: eight colors including black.
 * hub75_refresh() scans the entire framebuffer once and RETURNS. Call it
 * repeatedly to maintain the image. Pixel drawing alone produces no signals.
 * This version uses CPU-driven GPIO and busy waits; it does NOT implement 
 * background refresh, timer interrupts, or DMA.
 *
 * MINIMAL main.c -- COPY THIS EXAMPLE INTO YOUR APPLICATION, NOT THIS HEADER
 * ------------------------------------------------------------------------
 * #include "cybsp.h"
 * #include "hub75.h"
 *
 * int main(void)
 * {
 *     if (cybsp_init() != CY_RSLT_SUCCESS) {
 *         for (;;) { }
 *     }
 *
 *     SystemCoreClockUpdate();
 *     if (hub75_init(SystemCoreClock) != HUB75_OK) {
 *         for (;;) { }  // Initialization errors leave OE HIGH (blank).
 *     }
 *
 *     hub75_fill(HUB75_RED);  // First confirm the same solid-red image.
 *
 *     for (;;) {
 *         hub75_refresh();   // REQUIRED continuously, even for a still image.
 *         // Short, nonblocking input/game work may go here.
 *     }
 * }
 *
 * DRAWING A SIMPLE GAME SCREEN
 * ----------------------------
 * After successful initialization, replace hub75_fill(HUB75_RED) with:
 *
 *     hub75_clear();
 *     hub75_fill_rect(2, 2, 60, 6, HUB75_WHITE); // Health-bar background.
 *     hub75_fill_rect(2, 2, 44, 6, HUB75_RED);   // Current health.
 *     hub75_fill_rect(2, 12, 30, 6, HUB75_BLUE); // Mana bar.
 *     hub75_set_pixel(32, 32, HUB75_YELLOW);
 *
 * Continue calling hub75_refresh() inside the main loop. Redraw the affected
 * rectangles when health/mana changes; do not clear/redraw every loop unless
 * necessary. Clear the old bar area before drawing a shorter bar.
 *
 * WIRING -- YOUR CONFIRMED RIBBON ORDER
 * ------------------------------------
 * Numbers below follow your wire list. Colors repeat along the ribbon.
 * X1/X2 numbers are EXPANSION HEADER pins, not MCU package/Arduino numbers.
 *
 * Wire  Color    Panel signal   MCU GPIO    Relax Kit header
 *   1   Brown    R1             P5.0        X2 pin 20
 *   2   Red      G1             P5.1        X2 pin 21
 *   3   Orange   B1             P5.2        X2 pin 22
 *   4   Yellow   DNC            --          LEAVE DISCONNECTED
 *   5   Green    R2             P5.3        X2 pin 23
 *   6   Blue     G2             P5.4        X2 pin 24
 *   7   Purple   B2             P5.5        X2 pin 25
 *   8   Grey     LE / E         P0.4        X1 pin 6
 *   9   White    LA / A         P0.0        X1 pin 10
 *  10   Black    LB / B         P0.1        X1 pin 7
 *  11   Brown    LC / C         P0.2        X1 pin 8
 *  12   Red      LD / D         P0.3        X1 pin 5
 *  13   Orange   CLK            P5.6        X2 pin 26
 *  14   Yellow   LAT            P5.7        X2 pin 27
 *  15   Green    OE             P5.10       X1 pin 33
 *  16   Blue     GND            GND         X1 pin 39 or 40
 *
 * Use the panel's IN connector. LE is address E; LAT is the separate latch.
 * The panel's separate 5 V PSU powers the LEDs. Share ground with the MCU;
 * do not connect the panel's +5 V supply to a GPIO. MCU signals are 3.3 V.
 * This software does not provide level conversion or electrical isolation.
 * Make or change connections with both supplies off.
 *
 * COORDINATES AND COLORS
 * ----------------------
 * Coordinates are zero based: x = 0..63 and y = 0..63. The scan sends x in
 * ascending order. Verify physical left/right orientation with an asymmetric
 * drawing on your panel; an opposite shift direction can mirror the image.
 * For row-pair address n, RGB1 supplies y=n and RGB2 supplies y=n+32.
 * Out-of-bounds pixel writes are ignored. Rectangles are clipped to the panel.
 * Colors use bit 0=red, bit 1=green, bit 2=blue; only these three bits are kept.
 *
 * REFRESH AND TIMING
 * ------------------
 * A refresh call blocks for one complete scan: nominally 4.896 ms from the
 * explicit delays, PLUS instruction/function/interrupt overhead. Measure the
 * actual duration with your logic analyzer. It ends with OE HIGH, so the panel
 * is blank while the caller does other work. Long gaps, sleep calls, blocking
 * I2C/UART waits, or breakpoints make the display dim or flicker.
 *
 * Call refresh as often as practical. Enabled interrupts can still run, but
 * they disturb scan timing. Simply moving this whole function into an ISR
 * does not remove its CPU cost. This is a foreground driver for bring-up and
 * initial graphics; background refresh requires a separate scheduling design.
 *
 * Use one execution context for ALL driver calls. Do not draw from an ISR
 * while refresh reads the framebuffer. There is one buffer, no locking, and
 * no frame-swap API. Make drawing changes between refresh calls.
 *
 * The delay clock is the Cortex-M4 DWT cycle counter. Pass the CURRENT CPU
 * clock in hertz, not milliseconds or the CCU4 clock. Keep that clock fixed
 * after initialization, and do not disable the DWT counter while scanning.
 * No CCU4 timer, SysTick handler, heap allocation, or timer IRQ is required.
 *
 */

#ifndef HUB75_H
#define HUB75_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HUB75_WIDTH  64U
#define HUB75_HEIGHT 64U

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
    HUB75_ERROR_CLOCK,       /* cpu_hz outside 1 MHz..144 MHz. */
    HUB75_ERROR_COUNTER      /* DWT cycle counter did not advance. */
} hub75_status_t;

/* Call after cybsp_init() and SystemCoreClockUpdate(). Configures all listed
 * pins, blanks the LEDs, and clears the framebuffer to black. Reinitializing
 * also erases the image. Returns an error instead of hanging if its initial
 * clock/counter checks fail. OE is left HIGH on either error.
 */
hub75_status_t hub75_init(uint32_t cpu_hz);

/* Set all framebuffer pixels to black. Changes RAM only; keep refreshing. */
void hub75_clear(void);

/* Set every framebuffer pixel to color. Changes RAM only. */
void hub75_fill(hub75_color_t color);

/* Set one framebuffer pixel. Out-of-bounds coordinates are ignored. */
void hub75_set_pixel(int32_t x, int32_t y, hub75_color_t color);

/* Read one framebuffer pixel; out-of-bounds coordinates return BLACK. */
hub75_color_t hub75_get_pixel(int32_t x, int32_t y);

/* Fill width x height pixels starting at (x,y), clipping to the panel.
 * Zero/negative dimensions do nothing. Negative x/y are allowed for clipping.
 */
void hub75_fill_rect(int32_t x, int32_t y, int32_t width, int32_t height,
                     hub75_color_t color);

/* Scan all 32 row pairs once and RETURN, with LEDs blanked at the end.
 * Call continuously in the foreground. Does nothing unless init succeeded.
 */
void hub75_refresh(void);

/* Disable LED outputs, preserving image RAM. Does nothing before successful
 * initialization. A subsequent hub75_refresh() resumes displaying the image.
 */
void hub75_blank(void);

#ifdef __cplusplus
}
#endif

#endif /* HUB75_H */

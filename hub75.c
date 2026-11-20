/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 hub75-driver contributors
 * Distributed under the MIT License; see the full license in hub75.h.
 *
 * XMC4700 foreground HUB75 driver. Read hub75.h for the complete installation
 * example, ribbon-to-header wiring, API instructions, and timing limitations.
 *
 * The scan sequence is preserved from the working hub75_bringup.c test.
 * Instead of generating test patterns, this version transmits a framebuffer.
 */

#include "hub75.h"
#include "xmc_gpio.h"
#include <stdbool.h>

/* Fixed wiring: the bit packing below REQUIRES these contiguous GPIOs.
 * RGB1 = P5.0..2; RGB2 = P5.3..5; CLK = P5.6; LAT = P5.7; OE = P5.10.
 * Address A..E = P0.0..4. Moving wires requires changing both masks and packing.
 * P5.8/P5.9 (board LEDs) and other unrelated port bits are preserved.
 */
#define RGB_PORT       XMC_GPIO_PORT5
#define ADDR_PORT      XMC_GPIO_PORT0
#define RGB_MASK       UINT32_C(0x003f)
#define CLOCK_MASK     (UINT32_C(1) << 6)
#define LATCH_MASK     (UINT32_C(1) << 7)
#define OE_MASK        (UINT32_C(1) << 10)
#define ADDRESS_MASK   UINT32_C(0x001f)
#define ROW_PAIRS      (HUB75_HEIGHT / 2U)

/* Preserve the working test's initial timing. These are microseconds.
 * Each clock has a LOW data-setup interval and a HIGH interval, each at least
 * HALF_CLOCK_US long. Software overhead lengthens both. ROW_ON_US controls
 * the visible interval, while OE is LOW, for each selected pair of rows.
 * Increasing ROW_ON_US trades refresh rate for brightness. Measure before
 * changing these values; this implementation deliberately starts slowly.
 */
#define HALF_CLOCK_US  UINT32_C(1)
#define ROW_ON_US      UINT32_C(20)

/* One byte per pixel; only the bottom three bits are used. Static storage
 * requires no malloc and consumes exactly 4096 bytes for the image itself.
 * All drawing/refresh calls must run from a single execution context.
 */
static uint8_t framebuffer[HUB75_HEIGHT][HUB75_WIDTH];
static uint32_t ticks_per_us;
static bool initialized;

static uint8_t color_bits(hub75_color_t color)
{
    return (uint8_t)((uint32_t)color & UINT32_C(7));
}

/* Busy-wait against the running CPU cycle counter. This does not use SysTick
 * and does not reset CYCCNT, so an existing counter value is harmless.
 * Unsigned subtraction handles wrapping during these short delays. The
 * current constants (1, 2, and 20 us) keep the tick multiplication in range.
 */
static void delay_us(uint32_t us)
{
    const uint32_t start = DWT->CYCCNT;
    const uint32_t ticks = ticks_per_us * us;

    while ((uint32_t)(DWT->CYCCNT - start) < ticks) {
        /* CPU is occupied here; interrupts remain enabled. */
    }
}

/* An XMC OMR write sets pins using bits 0..15 and clears pins using 16..31.
 * Set/clear masks are disjoint, avoiding the register's toggle operation.
 * A zero in both halves leaves the corresponding output unchanged.
 * OMR is a write-command register: do not use |= or read-modify-write on it.
 */
static void write_mask(XMC_GPIO_PORT_t *port, uint32_t mask, uint32_t value)
{
    const uint32_t high = value & mask;
    const uint32_t low = mask & ~value;
    port->OMR = high | (low << 16);
}

static void init_output(XMC_GPIO_PORT_t *port, uint8_t pin,
                        XMC_GPIO_OUTPUT_LEVEL_t level)
{
    const XMC_GPIO_CONFIG_t config = {
        .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
        .output_level = level,
        /* Named equivalent of strength value 0 in the working test. */
        .output_strength = XMC_GPIO_OUTPUT_STRENGTH_STRONG_SHARP_EDGE
    };
    XMC_GPIO_Init(port, pin, &config);
}

/* One rising CLK edge transfers TWO pixels' channel bits in parallel.
 * upper bits R/G/B occupy P5.0/1/2; lower bits occupy P5.3/4/5.
 * Including CLOCK_MASK in the first write guarantees CLK is LOW while the
 * new RGB data is placed on the pins. RGB remains stable across the rising
 * edge and throughout CLK HIGH, satisfying setup and hold requirements.
 */
static void shift_column(uint8_t upper_rgb, uint8_t lower_rgb)
{
    const uint32_t data = (uint32_t)upper_rgb | ((uint32_t)lower_rgb << 3);

    write_mask(RGB_PORT, RGB_MASK | CLOCK_MASK, data);
    delay_us(HALF_CLOCK_US);
    RGB_PORT->OMR = CLOCK_MASK;
    delay_us(HALF_CLOCK_US);
    RGB_PORT->OMR = CLOCK_MASK << 16;
}

/* Draw the pair y=row and y=row+32. Keep OE HIGH while shifting, latching,
 * and changing the address. LAT commits the 64 shifted column samples to
 * the panel's output latches. Address bits A..E select the row pair in
 * ordinary binary; finally OE LOW allows those latched pixels to light.
 *
 * Expected analyzer capture at panel IN:
 *   CLK: 64 pulses per row pair, with RGB stable at each rising edge.
 *   LAT: one HIGH pulse after the 64th clock pulse.
 *   OE:  HIGH during shifting/address changes; LOW for about 20 us.
 *         Interrupts and software overhead can extend these intervals.
 */
static void scan_row(uint32_t row)
{
    RGB_PORT->OMR = OE_MASK;

    for (uint32_t x = 0; x < HUB75_WIDTH; ++x) {
        shift_column(framebuffer[row][x], framebuffer[row + ROW_PAIRS][x]);
    }

    delay_us(1);
    RGB_PORT->OMR = LATCH_MASK;
    delay_us(1);
    RGB_PORT->OMR = LATCH_MASK << 16;
    delay_us(1);
    write_mask(ADDR_PORT, ADDRESS_MASK, row);
    delay_us(2);

    RGB_PORT->OMR = OE_MASK << 16;
    delay_us(ROW_ON_US);
    RGB_PORT->OMR = OE_MASK;
}

hub75_status_t hub75_init(uint32_t cpu_hz)
{
    initialized = false;

    /* Establish blanking before configuring RGB, CLK, LAT, and addresses. */
    init_output(RGB_PORT, 10, XMC_GPIO_OUTPUT_LEVEL_HIGH);
    for (uint8_t pin = 0; pin <= 7; ++pin) {
        init_output(RGB_PORT, pin, XMC_GPIO_OUTPUT_LEVEL_LOW);
    }
    for (uint8_t pin = 0; pin < 5; ++pin) {
        init_output(ADDR_PORT, pin, XMC_GPIO_OUTPUT_LEVEL_LOW);
    }
    hub75_clear();

    /* XMC4700 is specified for CPU clocks up to 144 MHz. These checks catch
     * obvious unit/range errors; they cannot measure the true CPU frequency.
     */
    if (cpu_hz < UINT32_C(1000000) || cpu_hz > UINT32_C(144000000)) {
        return HUB75_ERROR_CLOCK;
    }
    ticks_per_us = (cpu_hz + UINT32_C(999999)) / UINT32_C(1000000);

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB();
    __ISB();

    const uint32_t before = DWT->CYCCNT;
    for (volatile uint32_t i = 0; i < 100; ++i) {
        __NOP();
    }
    if (DWT->CYCCNT == before) {
        return HUB75_ERROR_COUNTER;
    }

    initialized = true;
    return HUB75_OK;
}

void hub75_clear(void)
{
    hub75_fill(HUB75_BLACK);
}

void hub75_fill(hub75_color_t color)
{
    const uint8_t value = color_bits(color);
    for (uint32_t y = 0; y < HUB75_HEIGHT; ++y) {
        for (uint32_t x = 0; x < HUB75_WIDTH; ++x) {
            framebuffer[y][x] = value;
        }
    }
}

void hub75_set_pixel(int32_t x, int32_t y, hub75_color_t color)
{
    if (x < 0 || y < 0 || (uint32_t)x >= HUB75_WIDTH ||
        (uint32_t)y >= HUB75_HEIGHT) {
        return;
    }
    framebuffer[y][x] = color_bits(color);
}

hub75_color_t hub75_get_pixel(int32_t x, int32_t y)
{
    if (x < 0 || y < 0 || (uint32_t)x >= HUB75_WIDTH ||
        (uint32_t)y >= HUB75_HEIGHT) {
        return HUB75_BLACK;
    }
    return (hub75_color_t)framebuffer[y][x];
}

void hub75_fill_rect(int32_t x, int32_t y, int32_t width, int32_t height,
                     hub75_color_t color)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    /* Calculate ends in 64 bits so extreme signed coordinates cannot make
     * x+width or y+height overflow before clipping. Loop indices below are
     * converted only after the clipped values are known to lie in 0..64.
     */
    int64_t left = x;
    int64_t top = y;
    int64_t right = left + (int64_t)width;
    int64_t bottom = top + (int64_t)height;

    if (right <= 0 || bottom <= 0 || left >= HUB75_WIDTH ||
        top >= HUB75_HEIGHT) {
        return;
    }
    if (left < 0) { left = 0; }
    if (top < 0) { top = 0; }
    if (right > HUB75_WIDTH) { right = HUB75_WIDTH; }
    if (bottom > HUB75_HEIGHT) { bottom = HUB75_HEIGHT; }

    const uint8_t value = color_bits(color);
    for (uint32_t py = (uint32_t)top; py < (uint32_t)bottom; ++py) {
        for (uint32_t px = (uint32_t)left; px < (uint32_t)right; ++px) {
            framebuffer[py][px] = value;
        }
    }
}

void hub75_refresh(void)
{
    if (!initialized) {
        return;
    }
    for (uint32_t row = 0; row < ROW_PAIRS; ++row) {
        scan_row(row);
    }
    /* scan_row() blanks at the end of every row, including the final one. */
}

void hub75_blank(void)
{
    if (initialized) {
        RGB_PORT->OMR = OE_MASK;
    }
}

// pixel buffer and scanning for the 64x64 panel

#include "hub75.h"
#include "xmc_gpio.h"
#include <stdbool.h>

// RGB1 is P5.0..2, RGB2 is P5.3..5
// CLK = P5.6, LAT = P5.7, OE = P5.10, and A..E = P0.0..4
// only touch these bits so the board LEDs still work
#define RGB_PORT       XMC_GPIO_PORT5
#define ADDR_PORT      XMC_GPIO_PORT0
#define RGB_MASK       UINT32_C(0x003f)
#define CLOCK_MASK     (UINT32_C(1) << 6)
#define LATCH_MASK     (UINT32_C(1) << 7)
#define OE_MASK        (UINT32_C(1) << 10)
#define ADDRESS_MASK   UINT32_C(0x001f)
#define ROW_PAIRS      (HUB75_HEIGHT / 2U)

// delays are in us. Wait before raising CLK and before lowering it again
// ROW_ON_US controls how long the row stays on
// increasing it makes the panel brighter, but slows the full scan
#define HALF_CLOCK_US  UINT32_C(1)
#define ROW_ON_US      UINT32_C(20)

// one byte per pixel: bit 0 = red, bit 1 = green, bit 2 = blue
// 4096 bytes total. Update pixels between refresh calls
static uint8_t framebuffer[HUB75_HEIGHT][HUB75_WIDTH];
static uint32_t ticks_per_us;
static bool initialized;

static uint8_t color_bits(hub75_color_t color) {
    return (uint8_t)((uint32_t)color & UINT32_C(7));
}

// wait until enough CPU cycles have passed. Interrupts can still run here
// unsigned subtraction handles the counter wrapping during the wait
// keep these delays short so the tick calculation does not overflow
static void delay_us(uint32_t us) {
    const uint32_t start = DWT->CYCCNT;
    const uint32_t ticks = ticks_per_us * us;

    while ((uint32_t)(DWT->CYCCNT - start) < ticks) {
    }
}

// lower 16 OMR bits set pins, upper 16 clear them
// do not set and clear the same pin in one write, since that toggles it
// leave other pins alone. OMR is a command register, so do not read it back
static void write_mask(XMC_GPIO_PORT_t *port, uint32_t mask, uint32_t value) {
    const uint32_t high = value & mask;
    const uint32_t low = mask & ~value;
    port->OMR = high | (low << 16);
}

static void init_output(XMC_GPIO_PORT_t *port, uint8_t pin, XMC_GPIO_OUTPUT_LEVEL_t level) {
    const XMC_GPIO_CONFIG_t config = {
        .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
        .output_level = level,
        .output_strength = XMC_GPIO_OUTPUT_STRENGTH_STRONG_SHARP_EDGE
    };
    XMC_GPIO_Init(port, pin, &config);
}

// send one pixel for each half at the same time
// upper RGB goes in bits 0..2, lower RGB in 3..5
static void shift_column(uint8_t upper_rgb, uint8_t lower_rgb) {
    const uint32_t data = (uint32_t)upper_rgb | ((uint32_t)lower_rgb << 3);

    // change RGB with CLK low, then hold it while CLK is high
    write_mask(RGB_PORT, RGB_MASK | CLOCK_MASK, data);
    delay_us(HALF_CLOCK_US);
    RGB_PORT->OMR = CLOCK_MASK;
    delay_us(HALF_CLOCK_US);
    RGB_PORT->OMR = CLOCK_MASK << 16;
}

// row n is paired with row n+32
// should see 64 clocks, then a latch pulse. OE low turns the LEDs on
static void scan_row(uint32_t row) {
    // turn the LEDs off while loading the next row
    RGB_PORT->OMR = OE_MASK;

    for (uint32_t x = 0; x < HUB75_WIDTH; ++x) {
        shift_column(framebuffer[row][x], framebuffer[row + ROW_PAIRS][x]);
    }

    // latch the loaded pixels, then set the row address
    delay_us(1);
    RGB_PORT->OMR = LATCH_MASK;
    delay_us(1);
    RGB_PORT->OMR = LATCH_MASK << 16;
    delay_us(1);
    write_mask(ADDR_PORT, ADDRESS_MASK, row);
    delay_us(2); // give the row decoder time to settle

    // show this row pair, then turn it off before the next one
    RGB_PORT->OMR = OE_MASK << 16;
    delay_us(ROW_ON_US);
    RGB_PORT->OMR = OE_MASK;
}

hub75_status_t hub75_init(uint32_t cpu_hz) {
    initialized = false;

    // set OE high first so the panel stays off during setup
    init_output(RGB_PORT, 10, XMC_GPIO_OUTPUT_LEVEL_HIGH);
    for (uint8_t pin = 0; pin <= 7; ++pin) {
        init_output(RGB_PORT, pin, XMC_GPIO_OUTPUT_LEVEL_LOW);
    }
    for (uint8_t pin = 0; pin < 5; ++pin) {
        init_output(ADDR_PORT, pin, XMC_GPIO_OUTPUT_LEVEL_LOW);
    }
    hub75_clear();

    // check that the clock argument is reasonable
    // this does not measure it, so pass the updated SystemCoreClock value
    if (cpu_hz < UINT32_C(1000000) || cpu_hz > UINT32_C(144000000)) {
        return HUB75_ERROR_CLOCK;
    }
    ticks_per_us = (cpu_hz + UINT32_C(999999)) / UINT32_C(1000000);

    // enable the cycle counter without resetting it
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB();
    __ISB();

    // make sure the counter advances before relying on it for delays
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

void hub75_clear(void) {
    hub75_fill(HUB75_BLACK);
}

void hub75_fill(hub75_color_t color) {
    const uint8_t value = color_bits(color);
    for (uint32_t y = 0; y < HUB75_HEIGHT; ++y) {
        for (uint32_t x = 0; x < HUB75_WIDTH; ++x) {
            framebuffer[y][x] = value;
        }
    }
}

void hub75_set_pixel(int32_t x, int32_t y, hub75_color_t color) {
    if (x < 0 || y < 0 || (uint32_t)x >= HUB75_WIDTH || (uint32_t)y >= HUB75_HEIGHT) {
        return;
    }
    framebuffer[y][x] = color_bits(color);
}

hub75_color_t hub75_get_pixel(int32_t x, int32_t y) {
    if (x < 0 || y < 0 || (uint32_t)x >= HUB75_WIDTH || (uint32_t)y >= HUB75_HEIGHT) {
        return HUB75_BLACK;
    }
    return (hub75_color_t)framebuffer[y][x];
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
            framebuffer[py][px] = value;
        }
    }
}

void hub75_refresh(void) {
    if (!initialized) {
        return;
    }
    for (uint32_t row = 0; row < ROW_PAIRS; ++row) {
        scan_row(row);
    }
    // last row ends with OE high, so the panel is off when this returns
}

void hub75_blank(void) {
    if (initialized) {
        RGB_PORT->OMR = OE_MASK;
    }
}

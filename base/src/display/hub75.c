// scanning the framebuffer out to the chained 64x64 panels (XMC only)
// drawing into the framebuffer lives in hub75_fb.c

#include "hub75.h"
#include "hub75_fb.h"
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

static uint32_t ticks_per_us;
static bool initialized;

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
// should see 64 clocks per panel, then a latch pulse. OE low turns the LEDs on
static void scan_row(uint32_t row) {
    // turn the LEDs off while loading the next row
    RGB_PORT->OMR = OE_MASK;

    // send the whole chain in increasing column order: panel 0's data goes
    // first and ends up in the far panel (see HUB75_PANELS in hub75.h)
    for (uint32_t x = 0; x < HUB75_CHAIN_WIDTH; ++x) {
        shift_column(hub75_framebuffer[row][x], hub75_framebuffer[row + ROW_PAIRS][x]);
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
    hub75_fb_reset();

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

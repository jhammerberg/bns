// Scanning the front framebuffer out to the chained 64x64 panels (XMC only).
// A CCU40 timer interrupt sends one row pair per tick, so the display runs in
// the background. Drawing into the back framebuffer lives in hub75_fb.c.

#include "hub75.h"
#include "hub75_fb.h"
#include "xmc_ccu4.h"
#include "xmc_gpio.h"
#include "xmc_scu.h"

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

// Timing. Each interrupt shifts one row pair (128 columns), latches it and
// lights it for ROW_ON_US, so brightness = ROW_ON_US per row per frame.
#ifndef HUB75_FRAME_HZ
#define HUB75_FRAME_HZ    120  // full frames per second
#endif
#ifndef HUB75_HALF_CLOCK_NS
#define HUB75_HALF_CLOCK_NS 250 // shift clock half period: 2 MHz clock
#endif
#define ROW_ON_US         UINT32_C(20)

// Pin edge speed. Strong/sharp is what works on the current wiring: soft
// edges made the rows with address bit D (P0.3, an A2 pad) go missing
// (docs/bug-log.md).
// Note the same setting means different things per pad class (RM Table 26-6):
// on A1+ pads (P0.0-1, P5.0-2, P5.7, P5.10) "sharp" is already strong-soft.
#ifndef HUB75_PIN_STRENGTH
#define HUB75_PIN_STRENGTH XMC_GPIO_OUTPUT_STRENGTH_STRONG_SHARP_EDGE
#endif

#define TIMER_SLICE CCU40_CC40

static uint32_t ticks_per_us;
static uint32_t half_clock_ticks;
static bool initialized;

// written by the interrupt, read by the game loop
static volatile bool swap_pending;
static volatile bool blanked;
static volatile uint32_t frame_count;
static volatile uint32_t busy_us_total; // wraps after ~71 min; use differences
static uint32_t next_row;

static void delay_ticks(uint32_t ticks) {
    const uint32_t start = DWT->CYCCNT;
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
        .output_strength = HUB75_PIN_STRENGTH
    };
    XMC_GPIO_Init(port, pin, &config);
}

// send one pixel for each half at the same time
// upper RGB goes in bits 0..2, lower RGB in 3..5
static void shift_column(uint8_t upper_rgb, uint8_t lower_rgb) {
    const uint32_t data = (uint32_t)upper_rgb | ((uint32_t)lower_rgb << 3);

    // change RGB with CLK low, then hold it while CLK is high
    write_mask(RGB_PORT, RGB_MASK | CLOCK_MASK, data);
    delay_ticks(half_clock_ticks);
    RGB_PORT->OMR = CLOCK_MASK;
    delay_ticks(half_clock_ticks);
    RGB_PORT->OMR = CLOCK_MASK << 16;
}

// row n is paired with row n+32: 64 clocks per panel, a latch pulse, then
// OE low lights the row pair for ROW_ON_US
static void scan_row(uint32_t row) {
    const hub75_frame_t *frame = hub75_front;

    // turn the LEDs off while loading the next row
    RGB_PORT->OMR = OE_MASK;

    // send the whole chain in framebuffer order: the first columns sent end up
    // in the far panel (see HUB75_PANEL_COLUMN in hub75_fb.h)
    for (uint32_t x = 0; x < HUB75_CHAIN_WIDTH; ++x) {
        shift_column((*frame)[row][x], (*frame)[row + ROW_PAIRS][x]);
    }

    // latch the loaded pixels, then set the row address
    delay_ticks(ticks_per_us);
    RGB_PORT->OMR = LATCH_MASK;
    delay_ticks(ticks_per_us);
    RGB_PORT->OMR = LATCH_MASK << 16;
    delay_ticks(ticks_per_us);
    write_mask(ADDR_PORT, ADDRESS_MASK, row);
    delay_ticks(2 * ticks_per_us); // give the row decoder time to settle

    if (!blanked) {
        RGB_PORT->OMR = OE_MASK << 16;
        delay_ticks(ROW_ON_US * ticks_per_us);
        RGB_PORT->OMR = OE_MASK;
    }
}

// One row pair per timer tick. New frames are swapped in only before row 0,
// so a frame is always shown whole.
void CCU40_0_IRQHandler(void) {
    const uint32_t start = DWT->CYCCNT;
    XMC_CCU4_SLICE_ClearEvent(TIMER_SLICE, XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH);

    if (next_row == 0 && swap_pending) {
        hub75_fb_swap();
        swap_pending = false;
    }
    scan_row(next_row);
    if (++next_row == ROW_PAIRS) {
        next_row = 0;
        frame_count++;
    }
    busy_us_total += (DWT->CYCCNT - start) / ticks_per_us;
}

static void timer_start(void) {
    const uint32_t ccu_hz = XMC_SCU_CLOCK_GetCcuClockFrequency();
    const uint32_t period = ccu_hz / (HUB75_FRAME_HZ * ROW_PAIRS) - 1U; // 37499 at 144 MHz

    XMC_CCU4_Init(CCU40, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
    XMC_CCU4_StartPrescaler(CCU40);
    const XMC_CCU4_SLICE_COMPARE_CONFIG_t config = {
        .timer_mode = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
        .monoshot = XMC_CCU4_SLICE_TIMER_REPEAT_MODE_REPEAT,
        .prescaler_initval = XMC_CCU4_SLICE_PRESCALER_1,
    };
    XMC_CCU4_SLICE_CompareInit(TIMER_SLICE, &config);
    XMC_CCU4_SLICE_SetTimerPeriodMatch(TIMER_SLICE, (uint16_t)(period > 0xFFFFU ? 0xFFFFU : period));
    XMC_CCU4_EnableShadowTransfer(CCU40, XMC_CCU4_SHADOW_TRANSFER_SLICE_0);
    XMC_CCU4_SLICE_EnableEvent(TIMER_SLICE, XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH);
    XMC_CCU4_SLICE_SetInterruptNode(TIMER_SLICE, XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH,
                                    XMC_CCU4_SLICE_SR_ID_0);
    NVIC_SetPriority(CCU40_0_IRQn, 2); // above SysTick (millis), which is lowest
    NVIC_EnableIRQ(CCU40_0_IRQn);
    XMC_CCU4_EnableClock(CCU40, 0);
    XMC_CCU4_SLICE_StartTimer(TIMER_SLICE);
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
    half_clock_ticks = ticks_per_us * HUB75_HALF_CLOCK_NS / 1000U;

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
    timer_start();
    return HUB75_OK;
}

void hub75_present(void) {
    if (!initialized) {
        return;
    }
    swap_pending = true;
    while (swap_pending) {
        // Spin, don't __WFI(): XMC4700 sleep mode (SCU SLEEPCR default) slows
        // the system clock and stops the CCU clock, which stalls the display
        // timer that would end this wait (docs/bug-log.md).
    }
}

void hub75_blank(bool blank) {
    blanked = blank;
    if (blank) {
        RGB_PORT->OMR = OE_MASK;
    }
}

void hub75_get_stats(uint32_t *frames, uint32_t *busy_us) {
    *frames = frame_count;
    *busy_us = busy_us_total;
}

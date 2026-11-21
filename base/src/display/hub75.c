// Scanning the front framebuffer out to the chained 64x64 panels (XMC only).
// A CCU40 timer runs the display in the background with two interrupts per
// row slot: at the start of the slot (period match) the LEDs go off and the
// next row pair is shifted in; near the end (compare match) the LEDs go on.
// Brightness = the time between the compare and the end of the slot.
// Drawing into the back framebuffer lives in hub75_fb.c.

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O2") // hot path: built optimised even in Debug builds
#endif

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

// Timing
#ifndef HUB75_FRAME_HZ
#define HUB75_FRAME_HZ      120 // full frames per second
#endif
#ifndef HUB75_HALF_CLOCK_NS
#define HUB75_HALF_CLOCK_NS 250 // shift clock half period: 2 MHz clock
#endif
#ifndef HUB75_ROW_ON_US
#define HUB75_ROW_ON_US     20  // how long each row pair is lit per frame
#endif

// Pin edge speed. Strong/sharp is what works on the current wiring: soft
// edges made the rows with address bit D (P0.3, an A2 pad) go missing
// (docs/bug-log.md).
// Note the same setting means different things per pad class (RM Table 26-6):
// on A1+ pads (P0.0-1, P5.0-2, P5.7, P5.10) "sharp" is already strong-soft.
#ifndef HUB75_PIN_STRENGTH
#define HUB75_PIN_STRENGTH XMC_GPIO_OUTPUT_STRENGTH_STRONG_SHARP_EDGE
#endif

#define TIMER_SLICE CCU40_CC40

static uint32_t half_clock_ticks;
static uint32_t cycles_per_us;
static bool initialized;

// written by the interrupt, read by the game loop
static volatile bool swap_pending;
static volatile bool blanked;
static volatile uint32_t frame_count;
static volatile uint32_t busy_us_total; // wraps after ~71 min; use differences
static uint32_t next_row;

// Wait until the cycle counter reaches `deadline`. Deadlines are absolute, so
// the loop's own overhead doesn't add to the clock period.
static inline void wait_until(uint32_t deadline) {
    while ((int32_t)(DWT->CYCCNT - deadline) < 0) {
    }
}

static void init_output(XMC_GPIO_PORT_t *port, uint8_t pin, XMC_GPIO_OUTPUT_LEVEL_t level) {
    const XMC_GPIO_CONFIG_t config = {
        .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
        .output_level = level,
        .output_strength = HUB75_PIN_STRENGTH
    };
    XMC_GPIO_Init(port, pin, &config);
}

// OMR: the low 16 bits set pins, the high 16 clear them (only the pins named)

// Shift one row pair (row n and n+32) across the whole chain, then latch it.
// Columns go in framebuffer order: the first ones sent end up in the far
// panel (see HUB75_PANEL_COLUMN in hub75_fb.h).
static void shift_row(uint32_t row) {
    const uint8_t *upper = (*hub75_front)[row];
    const uint8_t *lower = (*hub75_front)[row + ROW_PAIRS];
    uint32_t t = DWT->CYCCNT;
    for (uint32_t x = 0; x < HUB75_CHAIN_WIDTH; ++x) {
        const uint32_t data = (uint32_t)upper[x] | ((uint32_t)lower[x] << 3);
        // new RGB with CLK low; the panel samples on the rising edge
        RGB_PORT->OMR = data | (((~data & RGB_MASK) | CLOCK_MASK) << 16);
        t += half_clock_ticks;
        wait_until(t);
        RGB_PORT->OMR = CLOCK_MASK;
        t += half_clock_ticks;
        wait_until(t);
    }
    RGB_PORT->OMR = CLOCK_MASK << 16;
    RGB_PORT->OMR = LATCH_MASK;
    wait_until(DWT->CYCCNT + half_clock_ticks);
    RGB_PORT->OMR = LATCH_MASK << 16;
}

// Start of a row slot: LEDs off, address the next row pair (it settles while
// shifting), shift it in. New frames are swapped in only before row 0, so a
// frame is always shown whole.
void CCU40_0_IRQHandler(void) {
    const uint32_t start = DWT->CYCCNT;
    RGB_PORT->OMR = OE_MASK;
    XMC_CCU4_SLICE_ClearEvent(TIMER_SLICE, XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH);

    if (next_row == 0 && swap_pending) {
        hub75_fb_swap();
        swap_pending = false;
    }
    ADDR_PORT->OMR = next_row | ((~next_row & ADDRESS_MASK) << 16);
    shift_row(next_row);
    if (++next_row == ROW_PAIRS) {
        next_row = 0;
        frame_count++;
    }
    busy_us_total += (DWT->CYCCNT - start) / cycles_per_us;
}

// Near the end of the slot: light the row pair until the next slot starts.
// (If shifting ran late this just runs right after it: same row, shorter on.)
void CCU40_1_IRQHandler(void) {
    XMC_CCU4_SLICE_ClearEvent(TIMER_SLICE, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP);
    if (!blanked) {
        RGB_PORT->OMR = OE_MASK << 16;
    }
}

static void timer_start(void) {
    const uint32_t ccu_hz = XMC_SCU_CLOCK_GetCcuClockFrequency();
    uint32_t period = ccu_hz / (HUB75_FRAME_HZ * ROW_PAIRS) - 1U; // 37499 at 144 MHz
    if (period > 0xFFFFU) {
        period = 0xFFFFU;
    }
    const uint32_t on_ticks = (ccu_hz / 1000000U) * HUB75_ROW_ON_US;
    const uint32_t compare = on_ticks < period ? period - on_ticks : 1U;

    XMC_CCU4_Init(CCU40, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
    XMC_CCU4_StartPrescaler(CCU40);
    const XMC_CCU4_SLICE_COMPARE_CONFIG_t config = {
        .timer_mode = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
        .monoshot = XMC_CCU4_SLICE_TIMER_REPEAT_MODE_REPEAT,
        .prescaler_initval = XMC_CCU4_SLICE_PRESCALER_1,
    };
    XMC_CCU4_SLICE_CompareInit(TIMER_SLICE, &config);
    XMC_CCU4_SLICE_SetTimerPeriodMatch(TIMER_SLICE, (uint16_t)period);
    XMC_CCU4_SLICE_SetTimerCompareMatch(TIMER_SLICE, (uint16_t)compare);
    XMC_CCU4_EnableShadowTransfer(CCU40, XMC_CCU4_SHADOW_TRANSFER_SLICE_0);
    XMC_CCU4_SLICE_EnableEvent(TIMER_SLICE, XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH);
    XMC_CCU4_SLICE_SetInterruptNode(TIMER_SLICE, XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH,
                                    XMC_CCU4_SLICE_SR_ID_0);
    XMC_CCU4_SLICE_EnableEvent(TIMER_SLICE, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP);
    XMC_CCU4_SLICE_SetInterruptNode(TIMER_SLICE, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP,
                                    XMC_CCU4_SLICE_SR_ID_1);
    // same priority, so the two never interrupt each other; both above SysTick
    NVIC_SetPriority(CCU40_0_IRQn, 2);
    NVIC_SetPriority(CCU40_1_IRQn, 2);
    NVIC_EnableIRQ(CCU40_0_IRQn);
    NVIC_EnableIRQ(CCU40_1_IRQn);
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
    cycles_per_us = (cpu_hz + UINT32_C(999999)) / UINT32_C(1000000);
    half_clock_ticks = cycles_per_us * HUB75_HALF_CLOCK_NS / 1000U;

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

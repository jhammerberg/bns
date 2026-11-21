// HUB75 scan for the chained 64x64 panels (XMC only): DMA + binary code
// modulation, so the CPU does almost no work per pixel.
//
// Each row pair is sent 4 times per frame, once per bit-plane of the RGB444
// pixels (hub75_encode.c). Plane p is lit for base_on << p, which gives 16
// brightness levels per channel. While one plane is lit, DMA shifts the next
// one in (hub75_dma.c), paced by a timer at HUB75_WRITE_HZ.
//
// A plane is latched and lit once both have happened:
//   - its shift finished      (DMA block complete, GPDMA0_0_IRQHandler)
//   - the previous plane went dark (its on-timer ran out, CCU40_0_IRQHandler)
// Both handlers run at the same, highest priority, so they never race.
// Encoding the next row runs one level lower (ENCODE_IRQHandler), so it can
// never delay switching LEDs off.
//
// The previous CPU-driven scan is commit fc76e52 ("faster display irq").
// Drawing into the back framebuffer lives in hub75_fb.c.

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O2") // hot path: built optimised even in Debug builds
#endif

#include "hub75.h"
#include "hub75_dma.h"
#include "hub75_encode.h"
#include "hub75_fb.h"
#include "xmc_ccu4.h"
#include "xmc_gpio.h"
#include "xmc_scu.h"

// CLK = P5.6, LAT = P5.7, OE = P5.10, RGB on P5.0-5, row address A..E = P0.0-4
#define RGB_PORT     XMC_GPIO_PORT5
#define ADDR_PORT    XMC_GPIO_PORT0
#define ADDRESS_MASK UINT32_C(0x001f)

// Timing
#ifndef HUB75_WRITE_HZ
#define HUB75_WRITE_HZ  4000000 // port writes per second: 2 per column -> 2 MHz shift clock
#endif
#ifndef HUB75_WHITE_ON_US
#define HUB75_WHITE_ON_US 20    // total lit time per row per frame for full brightness
#endif

// Pin edge speed. Strong/sharp is what works on the current wiring: soft
// edges made the rows with address bit D (P0.3, an A2 pad) go missing
// (docs/bug-log.md). The same setting means different things per pad class
// (RM Table 26-6): on A1+ pads "sharp" is already strong-soft.
#ifndef HUB75_PIN_STRENGTH
#define HUB75_PIN_STRENGTH XMC_GPIO_OUTPUT_STRENGTH_STRONG_SHARP_EDGE
#endif

// one-shot on-timers, one per plane, each preset to its own length
static XMC_CCU4_SLICE_t *const on_timer[HUB75_PLANES] = {CCU40_CC40, CCU40_CC41, CCU40_CC42, CCU40_CC43};

// the encoder runs in an otherwise unused interrupt, triggered from software
#define ENCODE_IRQn       CCU43_0_IRQn
#define ENCODE_IRQHandler CCU43_0_IRQHandler

// encoded row pairs, ping-pong by row parity: row r is in rows[r & 1]
static hub75_row_planes_t rows[2];

static uint32_t cycles_per_us;
static bool initialized;

// scan state (owned by the two timing interrupts)
static bool shifted;          // the plane to latch next is in the panel's shift registers
static bool dark;             // the previously lit plane has gone dark
static uint32_t next_row;     // the plane to latch next
static uint32_t next_plane;
static volatile uint32_t encode_row; // set before triggering the encoder

// shared with the game loop
static volatile bool swap_pending;
static volatile bool blanked;
static volatile uint32_t frame_count;
static volatile uint32_t busy_us_total; // wraps after ~71 min; use differences

static void init_output(XMC_GPIO_PORT_t *port, uint8_t pin, XMC_GPIO_OUTPUT_LEVEL_t level) {
    const XMC_GPIO_CONFIG_t config = {
        .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
        .output_level = level,
        .output_strength = HUB75_PIN_STRENGTH
    };
    XMC_GPIO_Init(port, pin, &config);
}

// per-handler profile (cycles, calls): [0] DMA done, [1] LEDs off, [2] encode
volatile uint32_t hub75_prof_cycles[3], hub75_prof_calls[3];

static void count_busy(uint32_t start, int which) {
    const uint32_t cycles = DWT->CYCCNT - start;
    busy_us_total += cycles / cycles_per_us;
    hub75_prof_cycles[which] += cycles;
    hub75_prof_calls[which]++;
}

// Both conditions met: latch the shifted plane, light it, shift the next one.
static void show_next(void) {
    shifted = dark = false;
    const uint32_t row = next_row, plane = next_plane;

    if (plane == 0) { // new row pair: LEDs are off, so switch the row address
        ADDR_PORT->OMR = row | ((~row & ADDRESS_MASK) << 16);
    }
    RGB_PORT->OMR = HUB75_LATCH_MASK;
    __NOP(); __NOP(); __NOP(); __NOP();
    RGB_PORT->OMR = HUB75_LATCH_MASK << 16;
    if (!blanked) {
        RGB_PORT->OMR = HUB75_OE_MASK << 16; // OE low: lit
    }
    XMC_CCU4_SLICE_ClearTimer(on_timer[plane]);
    XMC_CCU4_SLICE_StartTimer(on_timer[plane]);

    // shift the plane after this one while this one is lit
    next_plane = plane + 1U;
    next_row = row;
    if (next_plane == HUB75_PLANES) {
        next_plane = 0;
        next_row = (row + 1U) % HUB75_ROW_PAIRS;
    }
    hub75_dma_start(rows[next_row & 1U][next_plane], HUB75_ROW_WORDS);

    // a new row pair is lit: encode the following one, ready before it's needed
    if (plane == 0) {
        encode_row = (row + 1U) % HUB75_ROW_PAIRS;
        NVIC_SetPendingIRQ(ENCODE_IRQn);
    }
}

// a plane's shift finished
void GPDMA0_0_IRQHandler(void) {
    const uint32_t start = DWT->CYCCNT;
    hub75_dma_acknowledge();
    shifted = true;
    if (dark) {
        show_next();
    }
    count_busy(start, 0);
}

// a plane's on-time ran out
void CCU40_0_IRQHandler(void) {
    const uint32_t start = DWT->CYCCNT;
    RGB_PORT->OMR = HUB75_OE_MASK; // OE high: dark
    for (uint32_t p = 0; p < HUB75_PLANES; ++p) {
        XMC_CCU4_SLICE_ClearEvent(on_timer[p], XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH);
    }
    dark = true;
    if (shifted) {
        show_next();
    }
    count_busy(start, 1);
}

// encode the next row pair; new frames are swapped in at row 0 only, so a
// frame is always shown whole
void ENCODE_IRQHandler(void) {
    const uint32_t start = DWT->CYCCNT;
    const uint32_t row = encode_row;
    if (row == 0) {
        if (swap_pending) {
            hub75_fb_swap();
            swap_pending = false;
        }
        frame_count++;
    }
    hub75_encode_row(hub75_front, row, &rows[row & 1U]);
    count_busy(start, 2);
}

static void on_timers_init(void) {
    const uint32_t ccu_hz = XMC_SCU_CLOCK_GetCcuClockFrequency();
    // full white = all 4 planes lit = 15 x base
    uint32_t base = (ccu_hz / 1000000U) * HUB75_WHITE_ON_US / 15U;
    if (base < 1U) {
        base = 1U;
    }

    XMC_CCU4_Init(CCU40, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
    XMC_CCU4_StartPrescaler(CCU40);
    const XMC_CCU4_SLICE_COMPARE_CONFIG_t config = {
        .timer_mode = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
        .monoshot = XMC_CCU4_SLICE_TIMER_REPEAT_MODE_SINGLE,
        .prescaler_initval = XMC_CCU4_SLICE_PRESCALER_1,
    };
    static const uint32_t shadow[HUB75_PLANES] = {
        XMC_CCU4_SHADOW_TRANSFER_SLICE_0, XMC_CCU4_SHADOW_TRANSFER_SLICE_1,
        XMC_CCU4_SHADOW_TRANSFER_SLICE_2, XMC_CCU4_SHADOW_TRANSFER_SLICE_3,
    };
    for (uint32_t p = 0; p < HUB75_PLANES; ++p) {
        uint32_t ticks = base << p;
        if (ticks > 0xFFFFU) {
            ticks = 0xFFFFU;
        }
        XMC_CCU4_SLICE_CompareInit(on_timer[p], &config);
        XMC_CCU4_SLICE_SetTimerPeriodMatch(on_timer[p], (uint16_t)ticks);
        XMC_CCU4_EnableShadowTransfer(CCU40, shadow[p]);
        XMC_CCU4_SLICE_EnableEvent(on_timer[p], XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH);
        XMC_CCU4_SLICE_SetInterruptNode(on_timer[p], XMC_CCU4_SLICE_IRQ_ID_PERIOD_MATCH,
                                        XMC_CCU4_SLICE_SR_ID_0);
        XMC_CCU4_EnableClock(CCU40, (uint8_t)p);
    }
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

    // this does not measure the clock, so pass the updated SystemCoreClock value
    if (cpu_hz < UINT32_C(1000000) || cpu_hz > UINT32_C(144000000)) {
        return HUB75_ERROR_CLOCK;
    }
    cycles_per_us = (cpu_hz + UINT32_C(999999)) / UINT32_C(1000000);

    // cycle counter, used for the busy-time statistic
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

    // encode row pair 0 (black) and shift its first plane; it is latched as
    // soon as that shift completes
    hub75_encode_init(rows, 2);
    hub75_encode_row(hub75_front, 0, &rows[0]);
    next_row = 0;
    next_plane = 0;
    shifted = false;
    dark = true;

    on_timers_init();
    hub75_dma_init(HUB75_WRITE_HZ);
    // timing interrupts share the top priority; the encoder sits below them
    NVIC_SetPriority(GPDMA0_0_IRQn, 1);
    NVIC_SetPriority(CCU40_0_IRQn, 1);
    NVIC_SetPriority(ENCODE_IRQn, 3);
    NVIC_EnableIRQ(GPDMA0_0_IRQn);
    NVIC_EnableIRQ(CCU40_0_IRQn);
    NVIC_EnableIRQ(ENCODE_IRQn);

    initialized = true;
    hub75_dma_start(rows[0][0], HUB75_ROW_WORDS);
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
        // (docs/bug-log.md).
    }
}

void hub75_blank(bool blank) {
    blanked = blank;
    if (blank) {
        RGB_PORT->OMR = HUB75_OE_MASK;
    }
}

void hub75_get_stats(uint32_t *frames, uint32_t *busy_us) {
    *frames = frame_count;
    *busy_us = busy_us_total;
}

#include "cybsp.h"
#include "system_XMC4700.h"
#include "timer.h"

static volatile uint32_t ms_ticks = 0;
static uint32_t cycles_per_us;

void timer_init(void) {
    if(SysTick_Config(SystemCoreClock / 1000U) != 0U) {
        for(;;) { }
    }
    // cycle counter for micros()
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    cycles_per_us = SystemCoreClock / 1000000U;
}

void SysTick_Handler(void) {
    ms_ticks++;
}

uint32_t millis(void) {
    return ms_ticks;
}

uint32_t micros(void) {
    return DWT->CYCCNT / cycles_per_us;
}

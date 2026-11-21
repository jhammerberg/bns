#include "cybsp.h"
#include "system_XMC4700.h"
#include "timer.h"

static volatile uint32_t ms_ticks = 0;

void timer_init() {
    if(SysTick_Config(SystemCoreClock / 1000U) != 0U) {
        for(;;) { }
    }
}

void SysTick_Handler() {
    ms_ticks++;
}

uint32_t millis() {
    return ms_ticks;
}
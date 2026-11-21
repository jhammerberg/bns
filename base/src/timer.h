#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_init(void);

// milliseconds since timer_init (SysTick)
uint32_t millis(void);

// microseconds from the CPU cycle counter; wraps after ~29 s at 144 MHz, so
// only use it for short differences (e.g. how long one game frame took)
uint32_t micros(void);

#endif

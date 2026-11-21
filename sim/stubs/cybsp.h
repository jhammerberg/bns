// Laptop stand-in for the ModusToolbox BSP/XMCLib, just enough for
// base/src/main.c to compile. Add stubs here as main.c uses more hardware.

#ifndef SIM_CYBSP_H
#define SIM_CYBSP_H

#include <stdint.h>

typedef uint32_t cy_rslt_t;
#define CY_RSLT_SUCCESS ((cy_rslt_t)0)

extern uint32_t SystemCoreClock;
static inline void SystemCoreClockUpdate(void) {}
cy_rslt_t cybsp_init(void);

typedef struct { int unused; } XMC_USIC_CH_t;
#define CYBSP_DEBUG_UART_HW ((XMC_USIC_CH_t *)0)

// debug UART: bytes go to the sim's UART log
void XMC_UART_CH_Transmit(XMC_USIC_CH_t *channel, uint16_t data);

#endif // SIM_CYBSP_H

// Force-included into every file the sim compiles. On the board printf goes
// out the debug UART; here it goes to the sim's UART log instead of
// scribbling over the terminal display.

#ifndef SIM_FIRMWARE_H
#define SIM_FIRMWARE_H

#include <stdio.h>

int sim_uart_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
#define printf sim_uart_printf

#endif // SIM_FIRMWARE_H

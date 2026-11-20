#ifndef SIM_H
#define SIM_H

#include <stdbool.h>
#include <stdint.h>
#include "hub75.h"

// What the panels physically show, as of the last hub75_refresh().
typedef struct {
    uint8_t px[HUB75_HEIGHT][HUB75_CHAIN_WIDTH];
    bool lit;                 // false: not refreshed recently / blanked -> LEDs off
    uint32_t refreshes_per_s; // how often the firmware is calling hub75_refresh
} sim_glass_t;

// thread-safe copy of the current glass (called by the display loop)
void sim_glass_read(sim_glass_t *out);

// UART log: last few lines for the status area
void sim_uart_tail(char *out, size_t n, int lines);

// rendering
void render_terminal(const sim_glass_t *g, const char *status, const char *uart_tail);
int render_png(const char *path, const sim_glass_t *g);

uint64_t sim_now_us(void);

#endif // SIM_H

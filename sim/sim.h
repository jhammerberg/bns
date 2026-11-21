#ifndef SIM_H
#define SIM_H

#include <stdbool.h>
#include <stdint.h>
#include "hub75.h"

// What the panels physically show: the last presented frame.
typedef struct {
    uint8_t px[HUB75_HEIGHT][HUB75_CHAIN_WIDTH];
    bool lit;              // false before init or while blanked -> LEDs off
    bool stalled;          // game loop hasn't presented a frame for 100 ms
    uint32_t frames_per_s; // frames presented by the game loop
} sim_glass_t;

// thread-safe copy of the current glass (called by the display loop)
void sim_glass_read(sim_glass_t *out);

// keyboard -> wand shapes (input_sim.c); returns false for unmapped keys
bool sim_input_key(int key);

// one line of the receiver XIAO's USB log; queues the shape if it is one
bool sim_input_receiver_line(const char *line);

// UART log: last few lines for the status area
void sim_uart_tail(char *out, size_t n, int lines);

// rendering
void render_terminal(const sim_glass_t *g, const char *status, const char *uart_tail);
int render_png(const char *path, const sim_glass_t *g);

uint64_t sim_now_us(void);

#endif // SIM_H

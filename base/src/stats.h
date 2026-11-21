// Once-a-second health line on the debug serial port (J-Link VCOM on the
// board, sim/out/uart.log in the simulator), e.g.
//   fps 120 | display 31% cpu | game avg 420 us max 910 us | rx 10 B 2 ok 0 bad

#ifndef STATS_H
#define STATS_H

#include <stdbool.h>
#include <stdint.h>

// time spent on game work (input, tick, draw) in one loop, in microseconds
void stats_game_time(uint32_t us);

// receiver UART: bytes in, and packets accepted / rejected
void stats_rx_bytes(uint32_t count);
void stats_packet(bool ok);

// print the line if a second has passed since the last one
void stats_report(uint32_t now_ms);

#endif // STATS_H

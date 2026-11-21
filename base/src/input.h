#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include "protocol.h"

// Next shape cast with a wand, if any (never blocks). Call until it returns
// false. On the board these come from the receiver XIAO over UART; the
// simulator feeds them from the keyboard (sim/input_sim.c).
bool input_poll(bns_packet_t *out);

#endif // INPUT_H

#include "input.h"

bool input_poll(bns_packet_t *out) {
    (void)out;
    // TODO: read BNS_UART_SYNC-framed packets from the receiver over UART.
    return false;
}

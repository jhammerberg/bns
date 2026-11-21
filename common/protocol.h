/*
 * Wand -> receiver -> base packet format. Shared by base/ (XMC4700) and
 * esp/ (ESP32-C6); keep it plain C with fixed-width types only.
 *
 * Over the air (ESP-NOW) the payload is exactly one bns_packet_t.
 * Over UART (receiver -> base) each packet is framed as:
 *     BNS_UART_SYNC, then the bns_packet_t bytes.
 */

#ifndef BNS_PROTOCOL_H
#define BNS_PROTOCOL_H

#include <stdint.h>

#define BNS_PROTOCOL_VERSION 2
#define BNS_UART_SYNC        0xA5
#define BNS_UART_BAUD        115200
#define BNS_RADIO_CHANNEL    1     /* Wi-Fi channel all ESP-NOW boards use */

/* Shape drawn with the wand (held draw button), recognised on the wand. */
typedef enum {
    BNS_SHAPE_NONE     = 0, /* heartbeat / nothing recognised */
    BNS_SHAPE_TRIANGLE = 1,
    BNS_SHAPE_ZIGZAG   = 2,
    BNS_SHAPE_CIRCLE   = 3,
    BNS_SHAPE_V        = 4,
    BNS_SHAPE_COUNT
} bns_shape_t;

typedef struct __attribute__((packed)) {
    uint8_t version;   /* BNS_PROTOCOL_VERSION */
    uint8_t wand_id;   /* 1 or 2 */
    uint8_t seq;       /* increments per packet; receiver drops repeats */
    uint8_t shape;     /* bns_shape_t */
} bns_packet_t;

_Static_assert(sizeof(bns_packet_t) == 4, "bns_packet_t must stay 4 bytes");

#endif /* BNS_PROTOCOL_H */

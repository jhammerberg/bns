// Shapes from the receiver XIAO (esp/receiver) over UART.
//
// Wiring (docs/base-wiring.html): XIAO D6 (TX) -> Arduino D0 = P2.15
// (U1C0.DX0C), Arduino D1 = P2.14 (U1C0.DOUT0) -> XIAO D7, common GND.
// Each shape arrives as BNS_UART_SYNC followed by one bns_packet_t.
// A 32-byte hardware FIFO holds bytes between polls (one poll per frame).

#include "input.h"
#include "stats.h"
#include "xmc_gpio.h"
#include "xmc_uart.h"

#define UART_CH      XMC_UART1_CH0
#define UART_RX_PORT XMC_GPIO_PORT2
#define UART_RX_PIN  15
#define UART_TX_PORT XMC_GPIO_PORT2
#define UART_TX_PIN  14

void input_init(void) {
    const XMC_UART_CH_CONFIG_t config = {
        .baudrate = BNS_UART_BAUD,
        .data_bits = 8,
        .frame_length = 8,
        .stop_bits = 1,
        .oversampling = 16,
        .parity_mode = XMC_USIC_CH_PARITY_MODE_NONE,
    };
    XMC_UART_CH_Init(UART_CH, &config);
    XMC_UART_CH_SetInputSource(UART_CH, XMC_UART_CH_INPUT_RXD, USIC1_C0_DX0_P2_15);
    XMC_USIC_CH_RXFIFO_Configure(UART_CH, 0, XMC_USIC_CH_FIFO_SIZE_32WORDS, 1);
    XMC_UART_CH_Start(UART_CH);

    const XMC_GPIO_CONFIG_t rx = {.mode = XMC_GPIO_MODE_INPUT_PULL_UP};
    XMC_GPIO_Init(UART_RX_PORT, UART_RX_PIN, &rx);
    const XMC_GPIO_CONFIG_t tx = {
        .mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT2,
        .output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH,
    };
    XMC_GPIO_Init(UART_TX_PORT, UART_TX_PIN, &tx);
}

// Frame parser: wait for the sync byte, then collect the packet bytes.
static uint8_t frame[sizeof(bns_packet_t)];
static unsigned frame_len;
static bool in_frame;

static bool packet_valid(const bns_packet_t *p) {
    return p->version == BNS_PROTOCOL_VERSION && p->wand_id >= 1 && p->wand_id <= 2 &&
           p->shape > BNS_SHAPE_NONE && p->shape < BNS_SHAPE_COUNT;
}

bool input_poll(bns_packet_t *out) {
    while (!XMC_USIC_CH_RXFIFO_IsEmpty(UART_CH)) {
        const uint8_t byte = (uint8_t)XMC_USIC_CH_RXFIFO_GetData(UART_CH);
        stats_rx_bytes(1);
        if (!in_frame) {
            in_frame = byte == BNS_UART_SYNC;
            frame_len = 0;
            continue;
        }
        frame[frame_len++] = byte;
        if (frame_len < sizeof frame) {
            continue;
        }
        in_frame = false;
        bns_packet_t pkt;
        __builtin_memcpy(&pkt, frame, sizeof pkt);
        const bool ok = packet_valid(&pkt);
        stats_packet(ok);
        if (ok) {
            *out = pkt;
            return true;
        }
        // not a real packet (e.g. the ESP32 boot message): keep looking
    }
    return false;
}

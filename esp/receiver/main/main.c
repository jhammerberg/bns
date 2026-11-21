/*
 * Receiver: one XIAO at the base hears both wands over ESP-NOW and forwards
 * each recognised shape to the XMC4700 over UART as BNS_UART_SYNC +
 * bns_packet_t. Heartbeats (shape NONE) are logged, not forwarded.
 *
 * UART: XIAO D6 (GPIO16) TX -> XMC RX, D7 (GPIO17) RX <- XMC TX, common GND.
 * Note: the ESP32 ROM prints a boot message on GPIO16 at reset; the base
 * should resync on BNS_UART_SYNC and ignore anything else.
 */

#include <string.h>
#include "driver/uart.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "protocol.h"

#define BASE_UART    UART_NUM_1
#define BASE_UART_TX 16 // XIAO D6
#define BASE_UART_RX 17 // XIAO D7

static const char *TAG = "receiver";
static const char *const SHAPE_NAMES[BNS_SHAPE_COUNT] = {"none", "triangle", "zigzag", "circle", "v"};
static QueueHandle_t rx_queue;

// Runs in the Wi-Fi task: validate and hand off, nothing slow here.
static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    (void)info;
    if (len != sizeof(bns_packet_t)) {
        return;
    }
    bns_packet_t pkt;
    memcpy(&pkt, data, sizeof pkt);
    if (pkt.version != BNS_PROTOCOL_VERSION || pkt.wand_id < 1 || pkt.wand_id > 2 ||
        pkt.shape >= BNS_SHAPE_COUNT) {
        return;
    }
    xQueueSend(rx_queue, &pkt, 0);
}

static void radio_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(BNS_RADIO_CHANNEL, WIFI_SECOND_CHAN_NONE));

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_recv));
}

static void uart_init(void)
{
    const uart_config_t cfg = {
        .baud_rate = BNS_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(BASE_UART, 256, 256, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(BASE_UART, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(BASE_UART, BASE_UART_TX, BASE_UART_RX,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void app_main(void)
{
    rx_queue = xQueueCreate(16, sizeof(bns_packet_t));
    uart_init();
    radio_init();
    ESP_LOGI(TAG, "listening on channel %d", BNS_RADIO_CHANNEL);

    int last_seq[3] = {-1, -1, -1}; // index by wand_id
    unsigned heartbeats[3] = {0, 0, 0};
    bns_packet_t pkt;
    for (;;) {
        if (xQueueReceive(rx_queue, &pkt, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (pkt.seq == last_seq[pkt.wand_id]) {
            continue; // duplicate
        }
        last_seq[pkt.wand_id] = pkt.seq;

        if (pkt.shape == BNS_SHAPE_NONE) {
            // heartbeat: don't forward, just note the wand is alive every 5 s
            if (++heartbeats[pkt.wand_id] % 5 == 1) {
                ESP_LOGI(TAG, "wand %u alive (seq=%u)", pkt.wand_id, pkt.seq);
            }
            continue;
        }

        const uint8_t sync = BNS_UART_SYNC;
        uart_write_bytes(BASE_UART, &sync, 1);
        uart_write_bytes(BASE_UART, &pkt, sizeof pkt);
        ESP_LOGI(TAG, "wand %u seq=%u shape=%s", pkt.wand_id, pkt.seq,
                 pkt.shape < BNS_SHAPE_COUNT ? SHAPE_NAMES[pkt.shape] : "?");
    }
}

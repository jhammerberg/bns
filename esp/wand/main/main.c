/*
 * Wand: read the BNO055, turn motion into directions, send them over ESP-NOW.
 *
 * For now this broadcasts a test packet every 500 ms (cycling directions) so
 * the radio link can be checked before the IMU code is ported.
 * TODO: port base/src/imu.c to ESP-IDF I2C (XIAO D4=SDA GPIO22, D5=SCL GPIO23).
 */

#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "protocol.h"

static const char *TAG = "wand";
static const uint8_t BROADCAST[ESP_NOW_ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static void radio_init(void)
{
    esp_err_t err = nvs_flash_init(); // Wi-Fi stores calibration data in NVS
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
    esp_now_peer_info_t peer = {.channel = BNS_RADIO_CHANNEL, .ifidx = WIFI_IF_STA};
    memcpy(peer.peer_addr, BROADCAST, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
}

void app_main(void)
{
    radio_init();
    ESP_LOGI(TAG, "wand %d ready", CONFIG_BNS_WAND_ID);

    bns_packet_t pkt = {.version = BNS_PROTOCOL_VERSION, .wand_id = CONFIG_BNS_WAND_ID};
    for (;;) {
        pkt.direction = (uint8_t)((pkt.seq) % BNS_DIR_NONE);
        esp_err_t err = esp_now_send(BROADCAST, (const uint8_t *)&pkt, sizeof pkt);
        ESP_LOGI(TAG, "sent seq=%u dir=%u (%s)", pkt.seq, pkt.direction, esp_err_to_name(err));
        pkt.seq++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

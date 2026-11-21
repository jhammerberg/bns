/*
 * Wand: hold the draw button, draw a shape, release. The wand recognises the
 * shape (shape.c) and sends it to the receiver over ESP-NOW.
 *
 * LED (XIAO user LED): on while drawing; after release one long blink =
 * recognised and sent, three short blinks = not recognised.
 * USB log: one line per stroke. Raw IMU streaming for recording new shapes:
 * idf.py menuconfig -> BNS wand -> Stream raw IMU samples.
 *
 * Wiring (XIAO ESP32-C6 -> BNO055 breakout):
 *   3V3 -> VIN, GND -> GND, D10 (GPIO18) -> SDA, D9 (GPIO20) -> SCL
 *   draw button: D8 (GPIO19) -> button -> GND (internal pull-up)
 */

#include <string.h>
#include "bno055.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "protocol.h"
#include "shape.h"

#define IMU_SDA_GPIO     18 // XIAO D10
#define IMU_SCL_GPIO     20 // XIAO D9
#define IMU_PERIOD_MS    10 // 100 Hz, the BNO055 fusion output rate
#define DRAW_BUTTON_GPIO 19 // XIAO D8, low while the draw button is held
#define LED_GPIO         15 // XIAO user LED, wired from 3V3: low = on

static const char *TAG = "wand";
static const uint8_t BROADCAST[ESP_NOW_ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static uint8_t seq;
static shape_stroke_t stroke;

static void radio_init(void) {
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

static void send_shape(bns_shape_t shape) {
    const bns_packet_t pkt = {
        .version = BNS_PROTOCOL_VERSION,
        .wand_id = CONFIG_BNS_WAND_ID,
        .seq = seq++,
        .shape = (uint8_t)shape,
    };
    esp_err_t err = esp_now_send(BROADCAST, (const uint8_t *)&pkt, sizeof pkt);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "send failed: %s", esp_err_to_name(err));
    }
}

static void imu_connect(void) {
    uint8_t address;
    while (bno055_init(IMU_SDA_GPIO, IMU_SCL_GPIO, &address) != ESP_OK) {
        ESP_LOGE(TAG, "BNO055 not found, retrying in 1 s");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "BNO055 ready at 0x%02x", address);
}

static void led(bool on) {
    gpio_set_level(LED_GPIO, on ? 0 : 1);
}

static void blink(int times, int on_ms, int off_ms) {
    for (int i = 0; i < times; ++i) {
        led(true);
        vTaskDelay(pdMS_TO_TICKS(on_ms));
        led(false);
        vTaskDelay(pdMS_TO_TICKS(off_ms));
    }
}

static void gpio_setup(void) {
    const gpio_config_t button = {
        .pin_bit_mask = 1ULL << DRAW_BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&button));
    const gpio_config_t led_cfg = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&led_cfg));
    led(false);
}

// Time one recognition against every template, so the cost is visible.
static void benchmark(void) {
    shape_stroke_begin(&stroke);
    const int64_t t0 = esp_timer_get_time();
    shape_result_t r = shape_classify(shape_templates[0].pts, shape_templates,
                                      shape_template_count, 0);
    const int64_t t1 = esp_timer_get_time();
    ESP_LOGI(TAG, "recogniser: %d templates, one stroke takes %lld ms (self-check: %s)",
             shape_template_count, (t1 - t0) / 1000, shape_name(r.shape));
}

static void stroke_finished(void) {
    const int64_t t0 = esp_timer_get_time();
    const shape_result_t r = shape_recognize(&stroke);
    const int64_t t1 = esp_timer_get_time();
    ESP_LOGI(TAG, "stroke %d samples -> %s (closest %s, distance %.2f, %lld ms)",
             stroke.count, shape_name(r.shape), shape_name(r.best), r.distance, (t1 - t0) / 1000);
    if (r.shape != BNS_SHAPE_NONE) {
        send_shape(r.shape);
        blink(1, 400, 0);
    } else {
        blink(3, 80, 80);
    }
}

void app_main(void) {
    gpio_setup();
    radio_init();
    ESP_LOGI(TAG, "wand %d radio ready on channel %d", CONFIG_BNS_WAND_ID, BNS_RADIO_CHANNEL);
    imu_connect();
    benchmark();

#if CONFIG_BNS_WAND_LOG_IMU
    printf("imu,t_ms,draw,qw,qx,qy,qz,gyro_x,gyro_y,gyro_z,lin_x,lin_y,lin_z,"
           "cal_sys,cal_gyro,cal_accel,cal_mag,acc_x,acc_y,acc_z,grav_x,grav_y,grav_z\n");
#endif

    TickType_t wake = xTaskGetTickCount();
    int64_t last_heartbeat_us = 0;
    int errors = 0;
    bool drawing = false;
    for (;;) {
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(IMU_PERIOD_MS));

        bno055_sample_t s;
        if (bno055_read(&s) != ESP_OK) {
            if (++errors >= 10) { // lost the sensor: reinitialise
                ESP_LOGE(TAG, "BNO055 read failing, reconnecting");
                imu_connect();
                errors = 0;
            }
            continue;
        }
        errors = 0;

        const int64_t now_us = esp_timer_get_time();
        const bool draw = gpio_get_level(DRAW_BUTTON_GPIO) == 0;

#if CONFIG_BNS_WAND_LOG_IMU
        printf("imu,%lld,%d,%.4f,%.4f,%.4f,%.4f,%.1f,%.1f,%.1f,%.2f,%.2f,%.2f,%u,%u,%u,%u,"
               "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
               now_us / 1000, draw,
               s.quaternion.w, s.quaternion.x, s.quaternion.y, s.quaternion.z,
               s.gyro.x, s.gyro.y, s.gyro.z,
               s.linear_accel.x, s.linear_accel.y, s.linear_accel.z,
               s.calib_sys, s.calib_gyro, s.calib_accel, s.calib_mag,
               s.accel.x, s.accel.y, s.accel.z, s.gravity.x, s.gravity.y, s.gravity.z);
#endif

        if (draw && !drawing) {
            shape_stroke_begin(&stroke);
            led(true);
        }
        if (draw) {
            shape_stroke_add(&stroke, (uint32_t)(now_us / 1000), s.quaternion.w,
                             s.quaternion.x, s.quaternion.y, s.quaternion.z);
        }
        if (!draw && drawing) {
            led(false);
            stroke_finished();
            wake = xTaskGetTickCount(); // recognition/blink took a while; don't catch up
        }
        drawing = draw;

        if (!drawing && now_us - last_heartbeat_us >= 1000000) {
            send_shape(BNS_SHAPE_NONE); // link check for the receiver
            last_heartbeat_us = now_us;
        }
    }
}

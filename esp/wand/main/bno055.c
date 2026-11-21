// BNO055 over I2C using the ESP-IDF i2c_master driver.
// Register setup matches base/src/imu.c. Datasheet: reference/bno055/.

#include "bno055.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bno055";

enum {
    CHIP_ID = 0x00,
    PAGE_ID = 0x07,
    ACCEL = 0x08,
    MAG = 0x0e,
    GYRO = 0x14,
    EULER = 0x1a,
    QUATERNION = 0x20,
    LINEAR_ACCEL = 0x28,
    GRAVITY = 0x2e,
    TEMPERATURE = 0x34,
    CALIB_STAT = 0x35,
    SYS_TRIGGER = 0x3f,
    RST_SYS = 0x20,
    UNIT_SEL = 0x3b,
    OPR_MODE = 0x3d,
    PWR_MODE = 0x3e,
    CONFIG_MODE = 0x00,
    NDOF_MODE = 0x0c,
    BNO055_CHIP_ID = 0xa0,
};

#define I2C_HZ      100000 // BNO055 is most reliable at standard mode
#define I2C_TIMEOUT 50     // ms per transfer
// The BNO055 holds SCL low (clock stretching) while it works; allow for it.
#define SCL_WAIT_US 20000

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t dev;

static esp_err_t write_register(uint8_t reg, uint8_t value) {
    const uint8_t buf[2] = {reg, value};
    return i2c_master_transmit(dev, buf, sizeof buf, I2C_TIMEOUT);
}

static esp_err_t read_registers(uint8_t reg, uint8_t *data, size_t count) {
    return i2c_master_transmit_receive(dev, &reg, 1, data, count, I2C_TIMEOUT);
}

esp_err_t bno055_init(int sda_gpio, int scl_gpio, uint8_t *address_out) {
    const i2c_master_bus_config_t bus_cfg = {
        .i2c_port = -1,
        .sda_io_num = sda_gpio,
        .scl_io_num = scl_gpio,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, // breakout boards usually add their own
    };
    if (!bus) {
        ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &bus), TAG, "i2c bus");
    }

    // the BNO055 needs ~650 ms after power-up before it answers
    vTaskDelay(pdMS_TO_TICKS(650));

    uint8_t address = 0;
    if (i2c_master_probe(bus, 0x28, I2C_TIMEOUT) == ESP_OK) {
        address = 0x28;
    } else if (i2c_master_probe(bus, 0x29, I2C_TIMEOUT) == ESP_OK) {
        address = 0x29;
    } else {
        ESP_LOGE(TAG, "no device at 0x28 or 0x29 (check wiring, power, PS0/PS1 low)");
        return ESP_ERR_NOT_FOUND;
    }

    if (!dev) {
        const i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = address,
            .scl_speed_hz = I2C_HZ,
            .scl_wait_us = SCL_WAIT_US,
        };
        ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &dev), TAG, "add device");
    }

    uint8_t id = 0;
    ESP_RETURN_ON_ERROR(write_register(PAGE_ID, 0), TAG, "page select");
    ESP_RETURN_ON_ERROR(read_registers(CHIP_ID, &id, 1), TAG, "read chip id");
    if (id != BNO055_CHIP_ID) {
        ESP_LOGE(TAG, "chip id 0x%02x at 0x%02x, expected 0x%02x", id, address, BNO055_CHIP_ID);
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Full reset (same as power-on) so every boot starts from clean
    // calibration. The sensor self-calibrates while running and keeps any bad
    // offsets it learned until it loses power -- reflashing the ESP alone
    // does not clear them. It ignores I2C while it reboots (~650 ms).
    write_register(SYS_TRIGGER, RST_SYS); // may NACK as it starts resetting
    vTaskDelay(pdMS_TO_TICKS(650));
    id = 0;
    for (int i = 0; i < 20 && id != BNO055_CHIP_ID; ++i) {
        if (read_registers(CHIP_ID, &id, 1) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    if (id != BNO055_CHIP_ID) {
        ESP_LOGE(TAG, "no response after reset");
        return ESP_ERR_TIMEOUT;
    }

    ESP_RETURN_ON_ERROR(write_register(OPR_MODE, CONFIG_MODE), TAG, "config mode");
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_RETURN_ON_ERROR(write_register(PWR_MODE, 0), TAG, "normal power");
    ESP_RETURN_ON_ERROR(write_register(UNIT_SEL, 0x80), TAG, "units"); // m/s^2, deg/s, deg
    ESP_RETURN_ON_ERROR(write_register(OPR_MODE, NDOF_MODE), TAG, "ndof mode");
    vTaskDelay(pdMS_TO_TICKS(20));

    *address_out = address;
    return ESP_OK;
}

static int16_t read_i16(const uint8_t *b) {
    return (int16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
}

static bno055_vec3_t read_vec3(const uint8_t *b, float scale) {
    return (bno055_vec3_t){read_i16(b) * scale, read_i16(b + 2) * scale, read_i16(b + 4) * scale};
}

esp_err_t bno055_read(bno055_sample_t *s) {
    uint8_t d[CALIB_STAT - ACCEL + 1]; // one burst: ACCEL .. CALIB_STAT
    ESP_RETURN_ON_ERROR(read_registers(ACCEL, d, sizeof d), TAG, "read");

    const uint8_t *euler = d + (EULER - ACCEL);
    const uint8_t *quat = d + (QUATERNION - ACCEL);
    const uint8_t calib = d[CALIB_STAT - ACCEL];
    *s = (bno055_sample_t){
        .accel = read_vec3(d, 0.01f),
        .mag = read_vec3(d + (MAG - ACCEL), 1.0f / 16.0f),
        .gyro = read_vec3(d + (GYRO - ACCEL), 1.0f / 16.0f),
        .heading = read_i16(euler) / 16.0f,
        .roll = read_i16(euler + 2) / 16.0f,
        .pitch = read_i16(euler + 4) / 16.0f,
        .quaternion = {read_i16(quat) / 16384.0f, read_i16(quat + 2) / 16384.0f,
                       read_i16(quat + 4) / 16384.0f, read_i16(quat + 6) / 16384.0f},
        .linear_accel = read_vec3(d + (LINEAR_ACCEL - ACCEL), 0.01f),
        .gravity = read_vec3(d + (GRAVITY - ACCEL), 0.01f),
        .temperature = (int8_t)d[TEMPERATURE - ACCEL],
        .calib_sys = (calib >> 6) & 3,
        .calib_gyro = (calib >> 4) & 3,
        .calib_accel = (calib >> 2) & 3,
        .calib_mag = calib & 3,
    };
    return ESP_OK;
}

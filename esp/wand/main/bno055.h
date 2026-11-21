// BNO055 9-axis IMU over I2C (ESP-IDF port of base/src/imu.c).

#ifndef BNO055_H
#define BNO055_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

typedef struct {
    float x, y, z;
} bno055_vec3_t;

typedef struct {
    float w, x, y, z;
} bno055_quat_t;

typedef struct {
    bno055_vec3_t accel;        // m/s^2, includes gravity
    bno055_vec3_t mag;          // uT
    bno055_vec3_t gyro;         // deg/s
    float heading, roll, pitch; // deg
    bno055_quat_t quaternion;
    bno055_vec3_t linear_accel; // m/s^2, gravity removed
    bno055_vec3_t gravity;      // m/s^2
    int8_t temperature;         // degC
    // calibration 0 (none) .. 3 (full); fusion output is poor until sys > 0
    uint8_t calib_sys, calib_gyro, calib_accel, calib_mag;
} bno055_sample_t;

// Probe 0x28 then 0x29, check the chip ID, and start NDOF fusion mode.
// Returns the address found in *address_out.
esp_err_t bno055_init(int sda_gpio, int scl_gpio, uint8_t *address_out);

esp_err_t bno055_read(bno055_sample_t *sample);

#endif // BNO055_H

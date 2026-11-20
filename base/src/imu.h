#ifndef IMU_H
#define IMU_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float x, y, z;
} imu_vec3_t;

typedef struct {
    float w, x, y, z;
} imu_quat_t;

typedef struct {
    imu_vec3_t accel;          // m/s²
    imu_vec3_t mag;            // µT
    imu_vec3_t gyro;           // degrees/s
    float heading, roll, pitch; // degrees
    imu_quat_t quaternion;
    imu_vec3_t linear_accel;   // m/s²
    imu_vec3_t gravity;        // m/s²
    int8_t temperature;       // °C
} imu_sample_t;

bool imu_init(uint8_t address);
bool imu_read(imu_sample_t *sample);

#endif

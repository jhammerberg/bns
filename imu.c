#include "imu.h"
#include "cycfg_peripherals.h"

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
    UNIT_SEL = 0x3b,
    OPR_MODE = 0x3d,
    PWR_MODE = 0x3e,
    CONFIG_MODE = 0x00,
    NDOF_MODE = 0x0c
};

static XMC_USIC_CH_t *const bus = usic_0_ch_1_HW;
static uint16_t bus_address;

static void wait_flag(uint32_t flags)
{
    while ((XMC_I2C_CH_GetStatusFlag(bus) & flags) == 0) {}
}

static bool ack_received(void)
{
    const uint32_t ack = XMC_I2C_CH_STATUS_FLAG_ACK_RECEIVED;
    const uint32_t nack = XMC_I2C_CH_STATUS_FLAG_NACK_RECEIVED;
    wait_flag(ack | nack);
    bool ok = (XMC_I2C_CH_GetStatusFlag(bus) & nack) == 0;
    XMC_I2C_CH_ClearStatusFlag(bus, ack | nack);
    return ok;
}

static bool select_register(uint8_t reg)
{
    XMC_I2C_CH_ClearStatusFlag(bus, XMC_I2C_CH_STATUS_FLAG_ACK_RECEIVED |
                                  XMC_I2C_CH_STATUS_FLAG_NACK_RECEIVED);
    XMC_I2C_CH_MasterStart(bus, bus_address, XMC_I2C_CH_CMD_WRITE);
    if (!ack_received()) {
        return false;
    }
    XMC_I2C_CH_MasterTransmit(bus, reg);
    return ack_received();
}

static void stop_transfer(void)
{
    const uint32_t stopped = XMC_I2C_CH_STATUS_FLAG_STOP_CONDITION_RECEIVED;
    XMC_I2C_CH_ClearStatusFlag(bus, stopped);
    XMC_I2C_CH_MasterStop(bus);
    wait_flag(stopped);
    XMC_DelayUs(2);
}

static bool write_register(uint8_t reg, uint8_t value)
{
    bool ok = select_register(reg);
    if (ok) {
        XMC_I2C_CH_MasterTransmit(bus, value);
        ok = ack_received();
    }
    stop_transfer();
    return ok;
}

static bool read_registers(uint8_t reg, uint8_t *data, unsigned count)
{
    const uint32_t received = XMC_I2C_CH_STATUS_FLAG_RECEIVE_INDICATION |
                              XMC_I2C_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION;
    bool ok = select_register(reg);
    if (ok) {
        XMC_I2C_CH_MasterRepeatedStart(bus, bus_address, XMC_I2C_CH_CMD_READ);
        ok = ack_received();
    }
    if (ok) {
        for (unsigned i = 0; i < count; ++i) {
            XMC_I2C_CH_ClearStatusFlag(bus, received);
            if (i + 1 < count) {
                XMC_I2C_CH_MasterReceiveAck(bus);
            } else {
                XMC_I2C_CH_MasterReceiveNack(bus);
            }
            wait_flag(received);
            data[i] = XMC_I2C_CH_GetReceivedData(bus);
        }
    }
    stop_transfer();
    return ok;
}

bool imu_init(uint8_t address)
{
    uint8_t id;
    bus_address = (uint16_t)address << 1;
    XMC_Delay(650);
    if (!write_register(PAGE_ID, 0) ||
        !read_registers(CHIP_ID, &id, 1) || id != 0xa0 ||
        !write_register(OPR_MODE, CONFIG_MODE)) {
        return false;
    }
    XMC_Delay(20);
    if (!write_register(PWR_MODE, 0) ||
        !write_register(UNIT_SEL, 0x80) ||
        !write_register(OPR_MODE, NDOF_MODE)) {
        return false;
    }
    XMC_Delay(60);
    return true;
}

static int16_t read_i16(const uint8_t *bytes)
{
    return (int16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static imu_vec3_t read_vec3(const uint8_t *bytes, float scale)
{
    return (imu_vec3_t){
        read_i16(bytes) * scale,
        read_i16(bytes + 2) * scale,
        read_i16(bytes + 4) * scale
    };
}

bool imu_read(imu_sample_t *sample)
{
    uint8_t data[TEMPERATURE - ACCEL + 1];
    if (!read_registers(ACCEL, data, sizeof(data))) {
        return false;
    }

    const uint8_t *euler = data + (EULER - ACCEL);
    const uint8_t *quat = data + (QUATERNION - ACCEL);
    *sample = (imu_sample_t){
        .accel = read_vec3(data, 0.01f),
        .mag = read_vec3(data + (MAG - ACCEL), 1.0f / 16.0f),
        .gyro = read_vec3(data + (GYRO - ACCEL), 1.0f / 16.0f),
        .heading = read_i16(euler) / 16.0f,
        .roll = read_i16(euler + 2) / 16.0f,
        .pitch = read_i16(euler + 4) / 16.0f,
        .quaternion = {
            read_i16(quat) / 16384.0f,
            read_i16(quat + 2) / 16384.0f,
            read_i16(quat + 4) / 16384.0f,
            read_i16(quat + 6) / 16384.0f
        },
        .linear_accel = read_vec3(data + (LINEAR_ACCEL - ACCEL), 0.01f),
        .gravity = read_vec3(data + (GRAVITY - ACCEL), 0.01f),
        .temperature = (int8_t)data[TEMPERATURE - ACCEL]
    };
    return true;
}

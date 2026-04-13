/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <driver/i2c_master.h>

class I2cDriver {
public:
    I2cDriver(i2c_master_bus_handle_t i2c_bus, uint8_t addr);

protected:
    i2c_master_dev_handle_t i2c_device_;

    void WriteReg(uint8_t reg, uint8_t value);
    uint8_t ReadReg(uint8_t reg);
    void ReadRegs(uint8_t reg, uint8_t* buffer, size_t length);
};

#endif // I2C_DRIVER_H

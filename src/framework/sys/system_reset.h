/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _SYSTEM_RESET_H
#define _SYSTEM_RESET_H

#include <driver/gpio.h>

class SystemReset {
public:
    SystemReset(gpio_num_t reset_nvs_pin, gpio_num_t reset_factory_pin); // 构造函数私有化
    void CheckButtons();

private:
    gpio_num_t reset_nvs_pin_;
    gpio_num_t reset_factory_pin_;

    void ResetNvsFlash();
    void ResetToFactory();
    void RestartInSeconds(int seconds);
};

#endif

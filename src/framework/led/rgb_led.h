/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LED_RGB==1

#ifndef _RGB_LED_H_
#define _RGB_LED_H_

#include "led.h"
#include <driver/gpio.h>
#include <atomic>
#include <cstdint>

#include "../sys/mutex.h"

/**
 * 三色LED
 */
class RgbLed : public Led {
 public:
    RgbLed(gpio_num_t r_pin, gpio_num_t g_pin, gpio_num_t b_pin, bool output_invert=false);
    virtual ~RgbLed();

    void TurnOn() override;
    void TurnOff() override;
    void SetBrightness(uint8_t brightness) override { }
    void SetColor(uint8_t r, uint8_t g, uint8_t b) override;

protected:
    void OnBlinkTimer() override;

 private:
    const gpio_num_t r_pin_;
    const gpio_num_t g_pin_;
    const gpio_num_t b_pin_;
    const bool output_invert_ = false;

    Mutex *mutex_;
    uint8_t r_val_ = 0;
    uint8_t g_val_ = 0;
    uint8_t b_val_ = 0;


};

#endif  // _RGB_LED_H_

#endif //CONFIG_USE_LED_RGB
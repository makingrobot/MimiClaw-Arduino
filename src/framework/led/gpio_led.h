/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LED_GPIO==1

#ifndef _GPIO_LED_H_
#define _GPIO_LED_H_

#include "led.h"
#include <driver/gpio.h>
#include <atomic>
#include <cstdint>

#include "../sys/mutex.h"

/**
 * 单色LED
 */
class GpioLed : public Led {
 public:
    GpioLed(gpio_num_t gpio, bool pwm=true, bool output_invert=false);
    virtual ~GpioLed();

    void TurnOn() override;
    void TurnOff() override;
    void SetBrightness(uint8_t brightness) override;
    void SetColor(uint8_t r, uint8_t g, uint8_t b) override { }

protected:
    void OnBlinkTimer() override;

 private:
    const gpio_num_t led_pin_;
    const bool pwm_ = false;
    const bool output_invert_ = false;
    
    Mutex *mutex_;
    uint8_t brightness_ = 255;

};

#endif  // _GPIO_LED_H_

#endif //CONFIG_USE_LED_GPIO
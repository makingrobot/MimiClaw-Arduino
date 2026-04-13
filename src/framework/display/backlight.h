/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _BACKLIGHT_H
#define _BACKLIGHT_H

#include <cstdint>
#include <functional>
#include <driver/gpio.h>
#include "../sys/timer.h"

class Backlight {
public:
    Backlight();
    ~Backlight();

    void RestoreBrightness();
    void SetBrightness(uint8_t brightness, bool permanent = false);
    inline uint8_t brightness() const { return brightness_; }
    void OnTransitionTimer();

protected:
    virtual void SetBrightnessImpl(uint8_t brightness) = 0;

    Timer* transition_timer_ = nullptr;
    uint8_t brightness_ = 0;
    uint8_t target_brightness_ = 0;
    uint8_t step_ = 1;
};

/**
 * PWM背光
 * 采用渐进方式进行亮度调节
 */
class PwmBacklight : public Backlight {
public:
    /**
     * output_invert: 输出反转，默认是高电平点亮；若反转，则低电平为点亮。
     */
    PwmBacklight(gpio_num_t pin, bool output_invert = false);
    ~PwmBacklight();

    void SetBrightnessImpl(uint8_t brightness) override;
private:
    const gpio_num_t pin_;

};

#endif //_BACKLIGHT_H
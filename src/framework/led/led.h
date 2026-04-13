/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#include <cstdint>
#include "../sys/timer.h"
#include "../sys/log.h"

#ifndef _LED_H_
#define _LED_H_

#define DEFAULT_BRIGHTNESS      4
#define HIGH_BRIGHTNESS         16
#define LOW_BRIGHTNESS          2

#define BLINK_INFINITE          -1

#define TAG "Led"

/**
 * LED 基类
 */
class Led {
public:
    virtual ~Led() = default;

    /**
     * 点亮
     */
    virtual void TurnOn() = 0;

    /**
     * 熄灭
     */
    virtual void TurnOff() = 0;

    /**
     * 设置亮度，选用于PWM调光的LED
     * brightness: 0-255
     */
    virtual void SetBrightness(uint8_t brightness) = 0;

    /**
     * 设置颜色，适用于三色LED、灯珠等
     * r: 红色 0-255
     * g: 绿色 0-255
     * b: 蓝色 0-255
     */
    virtual void SetColor(uint8_t r, uint8_t g, uint8_t b) = 0;

    /**
     * 闪烁一次
     */
    void BlinkOnce() {
        Blink(1, 100);
    }

    /**
     * 闪烁指定次数
     * times: 次数，-1为无数次
     * interval_ms：间隔时间，毫秒
     */
    void Blink(int times, int interval_ms) {
        StartBlinkTask(times, interval_ms);
    }

protected:

    virtual void OnBlinkTimer() = 0;

    virtual void StartBlinkTask(int times, int interval_ms) {

        if (timer_==nullptr) 
        {
            Log::Error(TAG, "please create a timer in constructor.");
            return;
        }

        timer_->Stop();

        blink_counter_ = times * 2;
        blink_interval_ms_ = interval_ms;
        
        timer_->Start(interval_ms, [this](){ OnBlinkTimer(); });
    }

    int blink_counter_ = 0;
    int blink_interval_ms_ = 0;
    Timer* timer_ = nullptr;

};

/**
 * 无Led
 */
class NoLed : public Led {
public:
    void TurnOn() override { }
    void TurnOff() override { }
    void SetBrightness(uint8_t brightness) override { }
    void SetColor(uint8_t r, uint8_t g, uint8_t b) override { }

protected:
    void OnBlinkTimer() override { }
    
};

#endif // _LED_H_
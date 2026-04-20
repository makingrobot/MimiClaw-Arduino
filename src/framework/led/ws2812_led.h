/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LED_WS2812==1

#ifndef _WS2812_LED_H_
#define _WS2812_LED_H_

#include <driver/gpio.h>
#include <atomic>
#include <mutex>
#include <vector>

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "../sys/mutex.h"
#include "led.h"

/**
 * Ws2812灯珠类
 * 使用idf组件 led-strip 驱动
 */
class Ws2812Led : public Led {
public:
    Ws2812Led(gpio_num_t gpio, uint8_t num_pixels);
    virtual ~Ws2812Led();

    void TurnOn() override;
    void TurnOff() override;
    void SetColor(uint8_t r, uint8_t g, uint8_t b) override;
    void SetBrightness(uint8_t brightness) override { }
    
    uint8_t num_pixels() const { return num_pixels_; }
    
    /**
     * 设置要点亮灯珠的编号，编号范围：0-num_pixels
     */
    void SetLightNo(const std::vector<uint8_t>& light_set);
    
protected:
    void OnBlinkTimer() override;
    void StartBlinkTask(int times, int interval_ms) override;

private:
    const gpio_num_t pin_;
    const uint8_t num_pixels_;
    std::vector<uint8_t> light_set_ = { 0 };  // 默认只使用第1个灯珠

    Mutex *mutex_;
    uint8_t r_ = 255, g_ = 0, b_ = 0;  // 默认显示红色
    Adafruit_NeoPixel *pixels_ = nullptr;

    void Stop();
    
};

#endif // _WS2812_LED_H_

#endif //CONFIG_USE_LED_WS2812

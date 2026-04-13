/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LED_WS2812==1

#include "ws2812_led.h"
#include "../sys/log.h"
#include "../sys/mutex/std_mutex.h"
#include "../app/application.h"

#define TAG "Ws2812Led"

Ws2812Led::Ws2812Led(gpio_num_t pin, uint8_t num_pixels) : pin_(pin), num_pixels_(num_pixels) {
    mutex_ = new StdMutex();

    pixels_ = new Adafruit_NeoPixel(num_pixels, pin, NEO_GRB + NEO_KHZ800);
    pixels_->begin();

    timer_ = TimerFactory::CreateTimer("Ws2812_Led");
}

Ws2812Led::~Ws2812Led() {
    Stop();

    pixels_->clear();
}

void Ws2812Led::Stop() {
    if (timer_ != nullptr) {
        timer_->Stop();
    }
}

void Ws2812Led::SetColor(uint8_t r, uint8_t g, uint8_t b) {
    Log::Info(TAG, "Set color r:%d, g:%d, b:%d", r, g, b);
    r_ = r;
    g_ = g;
    b_ = b;
}

void Ws2812Led::TurnOn() {
    Log::Debug(TAG, "turn on");
    if (pixels_ == nullptr) {
        return;
    }

    MutexGuard lock(mutex_);
    if (lock.IsLocked())
    {
        Stop();

        pixels_->clear();
        for (uint8_t n : light_set_) {
            pixels_->setPixelColor(n, pixels_->Color(r_, g_, b_));
        }
        pixels_->show();
    }
}

void Ws2812Led::TurnOff() {
    Log::Debug(TAG, "turn off");
    if (pixels_ == nullptr) {
        return;
    }

    MutexGuard lock(mutex_);
    if (lock.IsLocked())
    {
        Stop();

        pixels_->clear();
        pixels_->show();
    }
}

void Ws2812Led::StartBlinkTask(int times, int interval_ms) {
    if (pixels_ == nullptr) {
        return;
    }

    Stop();
    
    blink_counter_ = times * 2;
    blink_interval_ms_ = interval_ms;
    
    timer_->Start(interval_ms, [this](){ 
        auto& app = Application::GetInstance();
        app.Schedule([this]() {
            OnBlinkTimer(); 
        });
    });
}

void Ws2812Led::OnBlinkTimer() {
    MutexGuard lock(mutex_);
    if (lock.IsLocked())
    {
        blink_counter_--;

        pixels_->clear();
        if (blink_counter_ & 1) {
            for (uint8_t n : light_set_) {
                pixels_->setPixelColor(n, pixels_->Color(r_, g_, b_));
            }
        } 
        pixels_->show();

        if (blink_counter_ == 0) {
            Stop();
        }
    }
}

void Ws2812Led::SetLightNo(const std::vector<uint8_t>& light_set) {
    light_set_ = {};
    
    for (uint8_t n : light_set) {
        if (n < num_pixels_) {
            light_set_.push_back(n);
        }
    }
}

#endif //CONFIG_USE_LED_WS2812
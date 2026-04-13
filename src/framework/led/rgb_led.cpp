/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LED_RGB==1

#include "rgb_led.h"
#include "../app/application.h"
#include "../sys/log.h"
#include "../sys/mutex/std_mutex.h"
#include <Arduino.h>

#define TAG "RgbLed"

// GPIO_LED
RgbLed::RgbLed(gpio_num_t r_pin, gpio_num_t g_pin, gpio_num_t b_pin, bool output_invert)
        : r_pin_(r_pin), g_pin_(g_pin), b_pin_(b_pin), output_invert_(output_invert) {

    assert(r_pin != GPIO_NUM_NC);
    assert(g_pin != GPIO_NUM_NC);
    assert(b_pin != GPIO_NUM_NC);

    mutex_ = new StdMutex();

    pinMode(r_pin, OUTPUT);
    pinMode(g_pin, OUTPUT);
    pinMode(b_pin, OUTPUT);

    timer_ = TimerFactory::CreateTimer("Rgb_Led");
}

RgbLed::~RgbLed() {
    if (timer_ != nullptr) {
        timer_->Stop();
    }
}

void RgbLed::SetColor(uint8_t r, uint8_t g, uint8_t b) {
    r_val_ = r;
    g_val_ = g;
    b_val_ = b;
}

void RgbLed::TurnOn() {
    MutexGuard lock(mutex_);
    if (lock.IsLocked())
    {
        blink_counter_ = 0;
        timer_->Stop();

        analogWrite(r_pin_, r_val_);
        analogWrite(g_pin_, g_val_);
        analogWrite(b_pin_, b_val_);
    }
}

void RgbLed::TurnOff() {
    MutexGuard lock(mutex_);
    if (lock.IsLocked())
    {
        blink_counter_ = 0;
        timer_->Stop();

        analogWrite(r_pin_, 0);
        analogWrite(g_pin_, 0);
        analogWrite(b_pin_, 0);
    }
}

void RgbLed::OnBlinkTimer() {
    
    MutexGuard lock(mutex_);
    if (lock.IsLocked())
    {
        blink_counter_--;
        if (blink_counter_ & 1) {
            analogWrite(r_pin_, r_val_);
            analogWrite(g_pin_, g_val_);
            analogWrite(b_pin_, b_val_);
        } else {
            analogWrite(r_pin_, 0);
            analogWrite(g_pin_, 0);
            analogWrite(b_pin_, 0);
        }

        if (blink_counter_ == 0) {
            timer_->Stop();
        }
    }
}

#endif //CONFIG_USE_LED_RGB
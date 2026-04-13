/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_LED_GPIO==1

#include "gpio_led.h"
#include "../app/application.h"
#include "../sys/log.h"
#include "../sys/mutex/std_mutex.h"
#include <Arduino.h>

#define TAG "GpioLed"

// GPIO_LED
GpioLed::GpioLed(gpio_num_t gpio, bool pwm, bool output_invert)
        : led_pin_(gpio), pwm_(pwm), output_invert_(output_invert) {

    assert(gpio != GPIO_NUM_NC);

    mutex_ = new StdMutex();

    pinMode(gpio, OUTPUT);
    timer_ = TimerFactory::CreateTimer("Gpio_Led");
}

GpioLed::~GpioLed() {
    if (timer_ != nullptr) {
        timer_->Stop();
    }
}

void GpioLed::SetBrightness(uint8_t brightness) {
    brightness_ = brightness;
    if (pwm_) {
        analogWrite(led_pin_, brightness);
    } else {
        digitalWrite(led_pin_, brightness > 127 ? HIGH : LOW);
    }
}

void GpioLed::TurnOn() {
    MutexGuard lock(mutex_);
    if (lock.IsLocked())
    {
        blink_counter_ = 0;
        timer_->Stop();

        if (pwm_) {
            analogWrite(led_pin_, output_invert_ ? 0 : brightness_);
        } else {
            digitalWrite(led_pin_, output_invert_ ? LOW : HIGH);
        }
    }
}

void GpioLed::TurnOff() {
    MutexGuard lock(mutex_);
    if (lock.IsLocked())
    {
        blink_counter_ = 0;
        timer_->Stop();

        if (pwm_) {
            analogWrite(led_pin_, output_invert_ ? brightness_ : 0);
        } else {
            digitalWrite(led_pin_, output_invert_ ? HIGH : LOW);
        }
    }
}

void GpioLed::OnBlinkTimer() {
    
    MutexGuard lock(mutex_);
    if (lock.IsLocked())
    {
        blink_counter_--;
        if (blink_counter_ & 1) {
            if (pwm_) {
                analogWrite(led_pin_, output_invert_ ? 0 : brightness_);
            } else {
                digitalWrite(led_pin_, output_invert_ ? LOW : HIGH);
            }
        } else {
            if (pwm_) {
                analogWrite(led_pin_, output_invert_ ? brightness_ : 0);
            } else {
                digitalWrite(led_pin_, output_invert_ ? HIGH : LOW);
            }
        }

        if (blink_counter_ == 0) {
            timer_->Stop();
        }
    }
}

#endif //CONFIG_USE_LED_GPIO
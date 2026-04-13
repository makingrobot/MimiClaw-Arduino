/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "backlight.h"

#include <driver/ledc.h>

#include "../sys/log.h"
#include "../sys/settings.h"
#include "../sys/timer.h"

#define TAG "Backlight"

Backlight::Backlight() {
    // 创建背光渐变定时器
    transition_timer_ = TimerFactory::CreateTimer("Backlight");
}

Backlight::~Backlight() {
    if (transition_timer_!=nullptr) {
        transition_timer_->Stop();
    }
}

void Backlight::RestoreBrightness() {
    // Load brightness from settings
    Settings settings("display");  
    int saved_brightness = settings.GetInt("brightness", 75);
    
    // 检查亮度值是否为0或过小，设置默认值
    if (saved_brightness <= 0) {
        Log::Warn( TAG, "Brightness value (%d) is too small, setting to default (10)", saved_brightness);
        saved_brightness = 10;  // 设置一个较低的默认值
    }
    
    SetBrightness(saved_brightness);
}

void Backlight::SetBrightness(uint8_t brightness, bool permanent) {
    if (brightness > 100) {
        brightness = 100;
    }

    if (brightness_ == brightness) {
        return;
    }

    if (permanent) {
        Settings settings("display", true);
        settings.SetInt("brightness", brightness);
    }

    target_brightness_ = brightness;
    step_ = (target_brightness_ > brightness_) ? 1 : -1;

    // 启动定时器，每 5ms 更新一次
    transition_timer_->Start(5, [this](){ OnTransitionTimer();} );
        
    Log::Info(TAG, "Set brightness to %d", brightness);
}

void Backlight::OnTransitionTimer() {
    if (brightness_ == target_brightness_) {
        transition_timer_->Stop();
        Log::Info(TAG, "transition timer stopped.");
        return;
    }

    brightness_ += step_;
    SetBrightnessImpl(brightness_);

    if (brightness_ == target_brightness_) {
        transition_timer_->Stop();
        Log::Info(TAG, "transition timer stopped.");
    }
}

PwmBacklight::PwmBacklight(gpio_num_t pin, bool output_invert) : Backlight(), pin_(pin) {
    const ledc_timer_config_t backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 25000, //背光pwm频率需要高一点，防止电感啸叫
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false
    };
    ESP_ERROR_CHECK(ledc_timer_config(&backlight_timer));
    
    // ledcSetup(LEDC_CHANNEL_0, /* channel */ 
    //         25000, /* freq */
    //         10 /* resolution */);
    
    // Setup LEDC peripheral for PWM backlight control
    const ledc_channel_config_t backlight_channel = {
        .gpio_num = pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .flags = {
            .output_invert = output_invert,
        }
    };
    ESP_ERROR_CHECK(ledc_channel_config(&backlight_channel));

    // ledcAttachPin(pin_, LEDC_CHANNEL_0);
    // if (output_invert) {
    //     ledcOutputInvert(pin_, output_invert);
    // }
}

PwmBacklight::~PwmBacklight() {
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    //ledcDetach(pin_);
}

void PwmBacklight::SetBrightnessImpl(uint8_t brightness) {
    // LEDC resolution set to 10bits, thus: 100% = 1023
    uint32_t duty_cycle = (1023 * brightness) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    //ledcWrite(LEDC_CHANNEL_0, duty_cycle);
}


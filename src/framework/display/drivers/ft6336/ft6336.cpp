/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "ft6336.h"
#include <esp_timer.h>
#include "src/framework/sys/log.h"
#include "src/framework/sys/timer.h"

#define TAG "Ft6336"

Ft6336::Ft6336(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : I2cDriver(i2c_bus, addr) {
    uint8_t chip_id = ReadReg(0xA3);
    Log::Info(TAG, "Get chip ID: 0x%02X", chip_id);
    read_buffer_ = new uint8_t[6];
    touchpad_timer_ = TimerFactory::CreateTimer("Ft6336");
}

Ft6336::~Ft6336() {
    if (touchpad_timer_!=nullptr) {
        touchpad_timer_->Stop();
    }
    delete[] read_buffer_;
}

void Ft6336::Start() {
    // 创建定时器，20ms 间隔
    touchpad_timer_->Start(20, [this](){PollTouchpad();} );
}

void Ft6336::UpdateTouchPoint() {
    ReadRegs(0x02, read_buffer_, 6);
    tp_.num = read_buffer_[0] & 0x0F;
    tp_.x = ((read_buffer_[1] & 0x0F) << 8) | read_buffer_[2];
    tp_.y = ((read_buffer_[3] & 0x0F) << 8) | read_buffer_[4];
}

void Ft6336::PollTouchpad() {
    static bool was_touched = false;
    static int64_t touch_start_time = 0;
    const int64_t TOUCH_THRESHOLD_MS = 500;  // 触摸时长阈值，超过500ms视为长按
    
    UpdateTouchPoint();
    auto& touch_point = GetTouchPoint();
    
    // 检测触摸开始
    if (touch_point.num > 0 && !was_touched) {
        was_touched = true;
        touch_start_time = esp_timer_get_time() / 1000; // 转换为毫秒
    } 
    // 检测触摸释放
    else if (touch_point.num == 0 && was_touched) {
        was_touched = false;
        int64_t touch_duration = (esp_timer_get_time() / 1000) - touch_start_time;
        
        // 只有短触才触发
        if (touch_duration < TOUCH_THRESHOLD_MS) {
            if (on_touched_) {
                on_touched_(touch_point);
            }
        }
    }
}
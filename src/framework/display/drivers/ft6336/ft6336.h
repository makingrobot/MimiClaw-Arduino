/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _FT6636_H
#define _FT6636_H

#include <driver/i2c_master.h>
#include "src/framework/board/i2c_driver.h"
#include "src/framework/types.h"
#include "src/framework/sys/timer.h"

class Ft6336 : public I2cDriver {
public:
    
    Ft6336(i2c_master_bus_handle_t i2c_bus, uint8_t addr);
    ~Ft6336();

    void Start();
    void PollTouchpad();

    void UpdateTouchPoint();

    void OnTouched(std::function<void(const TouchPoint_t&)> callback) {
        on_touched_ = callback;
    }

private:

    inline const TouchPoint_t& GetTouchPoint() {
        return tp_;
    }

    TouchPoint_t tp_;
    uint8_t* read_buffer_ = nullptr;
    Timer* touchpad_timer_ = nullptr;
    std::function<void(const TouchPoint_t&)> on_touched_;
};

#endif // _FT6636_H
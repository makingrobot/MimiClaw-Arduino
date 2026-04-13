/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _SINGLE_MOTOR_DRIVER_H
#define _SINGLE_MOTOR_DRIVER_H

#include "src/framework/peripheral/switch_actuator.h"

#define TAG "SingleMotorDriver"

/**
 * 单路电机驱动
 * 支持用两个引脚控制正反转的电机驱动芯片。
 */
class SingleMotorDriver : public Actuator {
public:
    SingleMotorDriver(gpio_num_t pin_in_a, gpio_num_t pin_in_b, bool output_invert=false) { 

        if (output_invert) {
            pin_in_a_ = pin_in_b;
            pin_in_b_ = pin_in_a;
        } else {
            pin_in_a_ = pin_in_a;
            pin_in_b_ = pin_in_b;
        }

        pinMode(pin_in_a_, OUTPUT);
        pinMode(pin_in_b_, OUTPUT);

        analogWrite(pin_in_a_, 0);
        analogWrite(pin_in_b_, 0);
    }

    virtual ~SingleMotorDriver() { 
        Off(); 
    };

    /*
     * 打开
     * power: 功率 128-255 太低会带不动电机。
     * reverse: 是否反向, 默认为false
     */
    void On(uint8_t power, bool reverse=false) {
        Log::Info(TAG, " On.");

        if (!reverse) {
            analogWrite(pin_in_a_, power);
            analogWrite(pin_in_b_, 0);
        } else {
            analogWrite(pin_in_a_, 0);
            analogWrite(pin_in_b_, power);
        }
        state_ = 1; //on
    }

    /**
     * 关闭
     */
    void Off() {
        analogWrite(pin_in_a_, 0);
        analogWrite(pin_in_b_, 0);

        state_ = 0; //off
        Log::Info(TAG, " Off.");
    }

    bool IsOn() {
        return state_==1;
    }

private:
    gpio_num_t pin_in_a_;
    gpio_num_t pin_in_b_;
    bool output_invert_ = false;
    
    uint8_t state_ = 0;
};

#endif //_SINGLE_MOTOR_DRIVER_H
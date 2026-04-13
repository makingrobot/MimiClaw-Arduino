/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _SWITCH_EXECUTOR_H
#define _SWITCH_EXECUTOR_H

#include <Arduino.h>
#include "actuator.h"

#define TAG "SwitchActuator"

/**
 * 开关类执行器，如继电器，小电机驱动等
 */
class SwitchActuator : public Actuator {
public:
    SwitchActuator(gpio_num_t pin, bool output_invert=false) 
            : sensor_pin_(pin), output_invert_(output_invert) { 

        pinMode(sensor_pin_, OUTPUT);
    }

    virtual ~SwitchActuator() { 
        Off(); 
    };

    virtual void On() {
        Log::Info(TAG, " On.");
        digitalWrite(sensor_pin_, output_invert_ ? 0 : 1);
        state_ = 1; //on
    }

    virtual void Off() {
        digitalWrite(sensor_pin_, output_invert_ ? 1 : 0);
        state_ = 0; //off
        Log::Info(TAG, " Off.");
    }

    void Switch() {
        state_==0 ? On() : Off();
    }

    bool IsOn() {
        return state_==1;
    }

private:
    const gpio_num_t sensor_pin_;
    const bool output_invert_;

    uint8_t state_ = 0;

};

#endif //_SWITCH_EXECUTOR_H
/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#if CONFIG_USE_SERVO==1

#ifndef _SERVO_DRIVER_H
#define _SERVO_DRIVER_H

#include <Arduino.h>
#include <ESP32Servo.h> //https://github.com/madhephaestus/ESP32Servo
#include "actuator.h"

#define TAG "ServoDriver"

/**
 * 舵机驱动
 */
class ServoDriver : public Actuator {
public:
    ServoDriver(gpio_num_t pin, uint16_t max_angle=180) 
            : servo_pin_(pin), max_angle_(180) { 

        servo_ = new Servo();
        servo_->attach(servo_pin_);
    }

    virtual ~ServoDriver() { 
        servo_->detach();
    };

    void SetAngle(uint16_t angle) {
        if (angle > max_angle_) {
            angle = max_angle_;
        }
        servo_->write(angle);
    }

    uint16_t GetAngle() {
        return servo_->read();
    }

    void Reset() {
        servo_->write(max_angle_ / 2); // 归位
    }

    void Move(uint16_t angle) {
        uint16_t new_angle = servo_->read() + angle;
        if (new_angle < 0) new_angle = 0;
        if (new_angle > max_angle_) new_angle = max_angle_;
        servo_->write(new_angle); 
    }

private:
    Servo *servo_;
    const gpio_num_t servo_pin_;
    const uint16_t max_angle_;

};

#endif //_SERVO_DRIVER_H

#endif
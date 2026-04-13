/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "sensor.h"
#include <Arduino.h>
#include "../sys/log.h"
#include "../sys/timer.h"
#include "../app/application.h"

#define TAG "Sensor"

Sensor::Sensor(const std::string& name) : name_(name) {
}

Sensor::~Sensor() {
    if (timer_ != nullptr) {
        timer_->Stop();
    }
}

void Sensor::Start(uint32_t interval_ms) {
    if (timer_ == nullptr) {
        timer_ = TimerFactory::CreateTimer("Sensor");
    } else {
        timer_->Stop();
    }
    
    timer_->Start(interval_ms, [this](){
        auto& app = Application::GetInstance();
        app.Schedule([this]() {
            ReadData();
        });
    });
}

void Sensor::Stop() {
    if (timer_ != nullptr) {
        timer_->Stop();
    }
}

/**
 * 读取传感器数据
 */
bool Sensor::ReadData() {

    bool success = ReadValue();
    if (success) {
        if (on_newdata_callback_) {
            on_newdata_callback_(*sensor_val_);
        } else {
            auto& app = Application::GetInstance();
            app.OnSensorDataEvent(name_, *sensor_val_);
        }
    }

    return success;
}

/*********** AnalogSensor **************/
AnalogSensor::AnalogSensor(const std::string& name, gpio_num_t pin) : Sensor(name),sensor_pin_(pin) {
    pinMode(sensor_pin_, INPUT);
}

bool AnalogSensor::ReadValue() {
    sensor_val_->SetIntValue(analogRead(sensor_pin_));
    return true;
}

/*********** DigitalSensor **************/
DigitalSensor::DigitalSensor(const std::string& name, gpio_num_t pin) : Sensor(name),sensor_pin_(pin) {
    pinMode(sensor_pin_, INPUT);
}

bool DigitalSensor::ReadValue() {
    sensor_val_->SetIntValue(digitalRead(sensor_pin_));
    return false;
}
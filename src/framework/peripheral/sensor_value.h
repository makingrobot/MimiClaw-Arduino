/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _SENSOR_VALUE_H
#define _SENSOR_VALUE_H

/**
 * 传感器数据
 */
class SensorValue {
public:
    const int intValue() const { return int_val_; }
    void SetIntValue(int int_val) { int_val_ = int_val; }
    
    const int floatValue() const { return float_val_; }
    void SetFloatValue(int float_val) { float_val_ = float_val; }
    
    const uint32_t ulongValue() const { return ulong_val_; }
    void SetUlongValue(uint32_t ulong_val) { ulong_val_ = ulong_val; }

private:
    int int_val_;

    float float_val_;
    
    uint32_t ulong_val_;
};

#endif //_SENSOR_VALUE_H
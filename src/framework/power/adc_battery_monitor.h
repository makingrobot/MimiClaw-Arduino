/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _ADC_BATTERY_MONITOR_H
#define _ADC_BATTERY_MONITOR_H

#include <functional>
#include <vector>
#include <driver/gpio.h>
#include "../sys/timer.h"

typedef struct {
    float voltage;                                        /*!< Battery voltage in volts */
    int capacity;                                         /*!< Battery capacity in percentage (0-100) */
} battery_point_t;

/**
 * 锂电池电量检测
 * 注意：本类默认电量与电压对应表仅适用于3.7V锂电池
 * 若是其他类型充电电池，请传入自定义的电量与电压对应表（battery_point_t[]）
 */
class AdcBatteryMonitor {
public:
    AdcBatteryMonitor(gpio_num_t charging_pin, float upper_resistor, float lower_resistor);
    AdcBatteryMonitor(gpio_num_t charging_pin, float upper_resistor, float lower_resistor, const battery_point_t* points);
    ~AdcBatteryMonitor();

    bool IsCharging();
    bool IsDischarging();
    int GetBatteryLevel();

    void OnChargingStatusChanged(std::function<void(bool)> callback);

protected:
    void CheckBatteryStatus();

private:
    const gpio_num_t charging_pin_;
    const float upper_resistor_, lower_resistor_; // 分压电阻
    const battery_point_t* battery_point_table_;

    int capacity_ = 1; // 电量百分比
    bool is_charging_ = false;
    std::function<void(bool)> on_charging_status_changed_;
    Timer* timer_ = nullptr;
    
};

#endif //_ADC_BATTERY_MONITOR_H

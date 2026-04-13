/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "adc_battery_monitor.h"
#include "../sys/log.h"
#include "../sys/timer.h"
#include "../app/application.h"

#include <Arduino.h>

#define TAG "AdcBatteryMonitor"

static const float Vref = 3.3;

static const battery_point_t battery_point_table[]={
    { 4.2 ,  100},
    { 4.06 ,  80},
    { 3.82 ,  60},
    { 3.58 ,  40},
    { 3.34 ,  20},
    { 3.1 ,  0},
    { 3.0 ,  -10}
};

static const battery_point_t battery_point_table2[] = {
    {4.16, 100},
    {4.07, 90},
    {3.99, 80},
    {3.90, 70},
    {3.82, 60},
    {3.72, 50},
    {3.61, 40},
    {3.53, 30},
    {3.38, 20},
    {3.20, 10},
    {2.85, 0},
};

AdcBatteryMonitor::AdcBatteryMonitor(gpio_num_t charging_pin, float upper_resistor, float lower_resistor)
    : AdcBatteryMonitor(charging_pin, upper_resistor, lower_resistor, battery_point_table) {

}

AdcBatteryMonitor::AdcBatteryMonitor(gpio_num_t charging_pin, float upper_resistor, float lower_resistor, const battery_point_t *point_table)
    : charging_pin_(charging_pin), upper_resistor_(upper_resistor), lower_resistor_(lower_resistor), battery_point_table_(point_table) {
    
    //analogSetWidth(12);

    timer_ = TimerFactory::CreateTimer("Battery_Monitor");
    timer_->Start(60*1000, [this](){
        auto& app = Application::GetInstance();
        app.Schedule([this]() {
            CheckBatteryStatus();
        });
    });
}

AdcBatteryMonitor::~AdcBatteryMonitor() {
    if (timer_ != nullptr) {
        timer_->Stop();
    }
}

bool AdcBatteryMonitor::IsCharging() {
    bool is_charging = gpio_get_level(charging_pin_) == 1;
    return is_charging;
}

bool AdcBatteryMonitor::IsDischarging() {
    // 没有区分充电和放电，所以直接返回相反状态
    return !is_charging_;
}

int AdcBatteryMonitor::GetBatteryLevel() {
    return capacity_;
}

void AdcBatteryMonitor::OnChargingStatusChanged(std::function<void(bool)> callback) {
    on_charging_status_changed_ = callback;
}

// Helper function to calculate battery capacity based on voltage
static float _calculate_battery_capacity(float voltage, const battery_point_t *points, size_t points_count)
{
    // Find the two points that bracket the current voltage
    size_t i;
    for (i = 0; i < points_count - 1; i++) {
        if (voltage >= points[i + 1].voltage && voltage <= points[i].voltage) {
            // Linear interpolation between the two points
            float voltage_range = points[i].voltage - points[i + 1].voltage;
            float capacity_range = points[i].capacity - points[i + 1].capacity;
            float voltage_offset = voltage - points[i + 1].voltage;

            return points[i + 1].capacity + (voltage_offset * capacity_range / voltage_range);
        }
    }

    // If voltage is outside the range, clamp to the nearest point
    if (voltage > points[0].voltage) {
        return points[0].capacity;
    } else {
        return points[points_count - 1].capacity;
    }
}

void AdcBatteryMonitor::CheckBatteryStatus() {
    Log::Debug(TAG, "Check battery status.");

    int adc_raw_value = analogRead(charging_pin_);
    float calc_vol = (adc_raw_value / 4095.0) * Vref;

    // 按分压电阻换算为实际值
    // vol = Vcc * (lower_resistor_ / (upper_resistor + lower_resistor))
    float origin_vol = calc_vol * (upper_resistor_+lower_resistor_) / lower_resistor_;

    capacity_ = _calculate_battery_capacity(origin_vol, battery_point_table_, sizeof(battery_point_table_) / sizeof(battery_point_t));
    
    Log::Debug(TAG, "Battery adc value: %.1f, capacity: %.1f%%", calc_vol, capacity_);

    bool new_charging_status = IsCharging();
    if (new_charging_status != is_charging_) {
        is_charging_ = new_charging_status;
        if (on_charging_status_changed_) {
            on_charging_status_changed_(is_charging_);
        }
    }
}

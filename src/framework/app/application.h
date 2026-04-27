/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _APPLICATION_H
#define _APPLICATION_H

#include <Arduino.h>

#include <string>
#include <deque>

#include "config.h"
#include "../ota/ota.h"
#include "../sys/timer.h"
#include "../sys/mutex.h"
#include "../sys/frt_task.h"
#include "../sys/frt_eventgroup.h"
#include "../peripheral/sensor_value.h"
#include "device_state.h"
#include "event_handler.h"
#include "../types.h"

void* create_application();

class Application {
public:
    static Application& GetInstance() {
        static Application* instance = static_cast<Application*>(create_application());
        return *instance;
    }

    Application();
    virtual ~Application();

    void Schedule(callback_function_t callback);
    DeviceState* GetDeviceState() { return device_state_; }
    void Alert(const char* status, const char* message, const char* emotion);
    void Reboot();

    // 初始化，在setup()中调用。
    void Init();

    // 循环，在loop()中调用。
    void Loop();

    virtual void SetDeviceState(const DeviceState* state);
    virtual void DismissAlert();
    virtual void ToggleWorkState();
    virtual void ShowWifiConfigHit(const std::string& ssid, const std::string& config_url, const std::string& mac_address);
    virtual bool CanEnterSleepMode();

    /**
     * 物理按键事件响应
     * button_name: 按键名称
     * action: 事件，单击/双击/长按...  
     */ 
    virtual bool OnPhysicalButtonEvent(const std::string& button_name, const ButtonAction action);

    /**
     * ESP32引脚触摸响应
     */
    virtual bool OnPinTouchEvent(const std::string& touch_name);

    /**
     * 显示屏触摸事件响应 
     * point: 触摸点坐标
     */ 
    virtual bool OnDisplayTouchEvent(const TouchPoint_t& point);

    /** 
     * 传感器数据事件响应
     * sensor_name: 传感器名称
     * value: 采集的数据，支持int/float/string/list
     */
    virtual bool OnSensorDataEvent(const std::string& sensor_name, const SensorValue& value) { }

    const std::string& GetLastErrorMessage() const { return last_error_message_; }

    virtual const std::string& GetAppVersion() const = 0;

#if CONFIG_CLOCK_ENABLE==1
    virtual void OnClockTimer();
#endif

protected:
    // 初始化事件.
    virtual bool OnInit() = 0;
    virtual void OnLoop() = 0;

    // 事件循环。
    void EventLoop();

    virtual void OnStateChanged();

#if CONFIG_OTA_ENABLE==1
    virtual void CheckNewVersion();
#endif

    DeviceState* device_state_ = const_cast<DeviceState*>(kDeviceStateUnknown);
    std::string last_error_message_;
    FrtEventGroup *event_group_;
    EventHandler *event_handler_ = nullptr;
    Mutex *mutex_;
    int clock_ticks_ = 0;

private:
    Application(const Application&) = delete; // 禁用拷贝构造函数
    Application& operator=(const Application&) = delete; // 禁用赋值操作

    std::deque<callback_function_t> app_tasks_;
    bool has_server_time_ = false;

    FrtTask *eventloop_task_;

#if CONFIG_CLOCK_ENABLE==1
    Timer* clock_timer_;
#endif

};

#endif //_APPLICATION_H

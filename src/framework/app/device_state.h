/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _DEVICE_STATE_H_
#define _DEVICE_STATE_H_

#include <string>
#include <map>

/**
 * 设备状态
 * 使用静态类实例来实现枚举效果
 */
class DeviceState {
private:
    DeviceState(int state, const std::string& text) : state_(state), text_(text) { }
    int state_;
    std::string text_;

public:

    DeviceState(const DeviceState& other) : state_(other.state_), text_(other.text_) { }

    DeviceState& operator=(const DeviceState& other) {
        this->state_ = other.state_;
        this->text_ = other.text_;
        return *this;
    }

    const bool operator==(const DeviceState& other) const {
        return this->state_ == other.state_;
    }
    const bool operator!=(const DeviceState& other) const {
        return this->state_ != other.state_;
    }

    const int state() const { return state_; }
    const std::string text() const { return text_; }

    static DeviceState* Create(int state, const std::string& text) {
        return new DeviceState(state, text);
    }
};

static const DeviceState* kDeviceStateFatalError         = DeviceState::Create(-2, "fatal_error");
static const DeviceState* kDeviceStateWarning            = DeviceState::Create(-1, "warning");
static const DeviceState* kDeviceStateUnknown            = DeviceState::Create(0, "unknown");
static const DeviceState* kDeviceStateIdle               = DeviceState::Create(1, "idle");
static const DeviceState* kDeviceStateStarting           = DeviceState::Create(2, "starting");
static const DeviceState* kDeviceStateWifiConfiguring    = DeviceState::Create(3, "configuring");
static const DeviceState* kDeviceStateConnecting         = DeviceState::Create(4, "connecting");
static const DeviceState* kDeviceStateWorking            = DeviceState::Create(5, "working");
static const DeviceState* kDeviceStateUpgrading          = DeviceState::Create(6, "upgrading");

#endif // _DEVICE_STATE_H_ 
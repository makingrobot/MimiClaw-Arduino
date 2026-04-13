/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _BOARD_H
#define _BOARD_H

#include <string>
#include <map>
#include <cstdint>
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>

#include "config.h"
#include "../led/led.h"
#include "../display/display.h"
#include "../display/disp_driver.h"
#include "../display/backlight.h"
#include "../audio/audio_codec.h"
#include "../camera/camera.h"
#include "../file/file_system.h"
#include "../sys/time.h"
#include "../types.h"
#include "../board/button.h"
#include "../peripheral/actuator.h"
#include "../peripheral/sensor.h"

void* create_board();

static const std::string kBootButton = "boot_button";

class Board {
private:
    Board(const Board&) = delete; // 禁用拷贝构造函数
    Board& operator=(const Board&) = delete; // 禁用赋值操作

    std::map<std::string, std::shared_ptr<Button>> button_map_; //按键列表
    std::map<std::string, std::shared_ptr<Actuator>> actuator_map_; //执行器外设列列表
    std::map<std::string, std::shared_ptr<Sensor>> sensor_map_; //传感器外设列表

protected:
    Board();
    std::string GenerateUuid();

    // 软件生成的设备唯一标识
    std::string uuid_;

    virtual void EnterSleepMode();
    virtual void ExitSleepMode();

    virtual void AddButton(std::shared_ptr<Button> button) {
        button_map_[button->name()] = button;
    }
    virtual void AddActuator(const std::string& name, std::shared_ptr<Actuator> actuator) {
        actuator_map_[name] = actuator;
    }
    virtual void AddSensor(std::shared_ptr<Sensor> sensor) {
        sensor_map_[sensor->name()] = sensor;
    }
   
    std::map<std::string, std::shared_ptr<Button>> button_map() const { return button_map_; };

public:
    static Board& GetInstance() {
        static Board* instance = static_cast<Board*>(create_board());
        return *instance;
    }

    // virtual methods
    virtual ~Board() = default;

    void Sleep(uint32_t time_ms);
    virtual void Shutdown();
    virtual std::string GetUuid() { return uuid_; }
    virtual std::string GetJson();
    virtual std::string GetBoardType() { return ""; }
    virtual std::string GetBoardJson() { return "{}"; }
    virtual std::string GetDeviceStatusJson() { return "{}"; }
    virtual void SetPowerSaveMode(bool enabled) { } 
    virtual bool GetTemperature(float& temp) { return false; }
    virtual bool GetBatteryLevel(int &level, bool& charging, bool& discharging) { return false; }

    virtual const char* GetNetworkStateIconName() { return ""; }

    virtual bool OnPhysicalButtonEvent(const std::string& button_name, const ButtonAction action);
    virtual bool OnDisplayTouchEvent(const TouchPoint_t& point);

    virtual Led* GetLed();

    virtual DispDriver* GetDispDriver() { return nullptr; }
    virtual Backlight* GetBacklight() { return nullptr; }
    virtual Time* GetTime() { return nullptr; }

    virtual Display* GetDisplay();
#if CONFIG_USE_LVGL==1
    virtual void SetDisplay(Display *display) = 0;
#endif

#if CONFIG_USE_AUDIO==1
    virtual AudioCodec* GetAudioCodec() { return nullptr; }
#endif

#if CONFIG_USE_FS==1
    virtual FileSystem* GetFileSystem() { return nullptr; }
#endif

#if CONFIG_USE_CAMERA==1
    virtual Camera* GetCamera() { return nullptr; }
#endif

    // 查找执行器外设
    std::shared_ptr<Actuator> GetActuator(const std::string& name);

    // 查找传感器外设
    std::shared_ptr<Sensor> GetSensor(const std::string& name);

};


static void I2cDetect(i2c_master_bus_handle_t bus) {
    uint8_t address;
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    for (int i = 0; i < 128; i += 16) {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++) {
            fflush(stdout);
            address = i + j;
            esp_err_t ret = i2c_master_probe(bus, address, pdMS_TO_TICKS(200));
            if (ret == ESP_OK) {
                printf("%02x ", address);
            } else if (ret == ESP_ERR_TIMEOUT) {
                printf("UU ");
            } else {
                printf("-- ");
            }
        }
        printf("\r\n");
    }
}

#endif  //_BOARD_H
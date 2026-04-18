#include "config.h"
#if BOARD_ESP32_S3_DEVKIT == 1

#include "board_config.h"
#include "esp32_s3_board.h"
#include "src/framework/led/gpio_led.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "src/mimiclaw/arduino_json_psram.h"
#include "src/mimiclaw/mimi_application.h"

#if CONFIS_USE_SPIFFS==1
#include <SPIFFS.h>
#elif CONFIG_USE_SDFS==1
#include <SPI.h>
#include <SD.h>
#endif

#define TAG "Esp32S3Board"

bool tool_led_execute(const char *in_json, char *out, size_t len);

void* create_board() { 
    return new Esp32S3Board();
}

Esp32S3Board::Esp32S3Board() : WifiBoard() {

    Log::Info(TAG, "===== Create Board ...... =====");

    Log::Info(TAG, "initial led.");
    led_ = new GpioLed(LED_PIN, 1);

    InitFileSystem();
    Log::Info( TAG, "===== Board config completed. =====");

    // 工具
    static const MimiTool led = {
        .name = "led",
        .description = "Turn on/off the LED immediately. Use when user says 'led on val / led off'.",
        .input_schema_json = "{\"type\":\"object\","
            "\"properties\":{"
                "\"action\":{\"type\":\"string\",\"description\":\"Action(on|off)\"},"
                "\"value\":{\"type\":\"integer\",\"description\":\"Value(0-255)\"}"
            "},"
            "\"required\":[\"action\"]}",
        .execute = tool_led_execute,  // 绑定执行函数
    };
    _tools.push_back(&led);

    InstallSkill();
}

void Esp32S3Board::InitFileSystem() {
    Log::Info( TAG, "Init file system ......" );

#if CONFIS_USE_SPIFFS==1
    if (!SPIFFS.begin()) {
        Log::Error(TAG, "SPIFFS Mount Failed");
        return;
    }

    filesystem_ = new FileSystem(SPIFFS);
    filesystem_->setTotalBytes(SPIFFS.totalBytes());
    filesystem_->setFreeBytes(SPIFFS.totalBytes() - SPIFFS.usedBytes());
    filesystem_->setType("SPIFFS");

#elif CONFIG_USE_SDFS==1
    SPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN); // config in board_config.h
    if (!SD.begin(SD_CS_PIN)) {
        Log::Info(TAG, "SD Mount Failed");
        return;
    }

    filesystem_ = new FileSystem(SD);
    filesystem_->setTotalBytes(SD.totalBytes());
    filesystem_->setFreeBytes(SD.totalBytes() - SD.usedBytes());
    filesystem_->setType("SDFS");
#endif
}

void Esp32S3Board::InstallSkill() {

    static const char LED_SKILL[] PROGMEM =
"# LED Control\n"
"Control led.\n"
"\n"
"## When to use\n"
"When the user asks to led turn on/off.\n"
"\n"
"## How to use\n"
"1. To **led on**: use led on with value(0-255)\n"
"2. To **led off**: use led off\n"
"\n"
"## Example\n"
"User: \"led on 255\"\n"
"→ led { \"action\": \"on\", \"value\": 255 }"
"\n"
"User: \"led off\"\n"
"→ led { \"action\": \"off\" })\n";

    MimiApplication *app = (MimiApplication*)(&Application::GetInstance());
    app->skills().installSkill("led-control", LED_SKILL);
}

//=================== Led Tool =============================
// Led on工具函数
bool tool_led_execute(const char *in_json, char *out, size_t len) {
    JsonDocument inDoc(&spiram_allocator);
    if (deserializeJson(inDoc, in_json)) {
        snprintf(out, len, "Error: invalid input");
        return false; //ESP_ERR_INVALID_ARG;
    }

    Led *led = Board::GetInstance().GetLed();
    if (led==nullptr) {
        snprintf(out, len, "Error: no led device");
        return false; 
    }

    const char *action = inDoc["action"] | (const char*)nullptr;
    uint8_t value = inDoc["value"].as<uint8_t>();
    
    if (strcmp(action, "on")==0) {
        led->TurnOn();
        snprintf(out, len, "LED ON");  // 执行结果
        return true;

    } else if (strcmp(action, "off")==0) {
        led->TurnOff();
        snprintf(out, len, "LED OFF");  // 执行结果
        return true;
    }

    snprintf(out, len, "Error: invalid action.");  // 不能识别的动作
    return false;
}

#endif
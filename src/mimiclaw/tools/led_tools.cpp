
#include "led_tools.h"
#include "driver/gpio.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include "src/mimiclaw/arduino_json_psram.h"
#include "src/framework/board/board.h"

//=================== Led Tool =============================
bool tool_led_execute(const char *input_json, char *output, size_t output_size) {
    JsonDocument inDoc(&spiram_allocator);
    if (deserializeJson(inDoc, input_json)) {
        snprintf(output, output_size, "Error: invalid input");
        return false;
    }

    Led *led = Board::GetInstance().GetLed();
    if (led==nullptr) {
        snprintf(output, output_size, "Error: no led device");
        return false; 
    }

    const char *action = inDoc["action"] | (const char*)nullptr;
    
    if (strcmp(action, "on")==0) {
        uint8_t brigntness = inDoc["value"].as<uint8_t>();
        led->SetBrightness(brigntness); // 设置亮度
        led->TurnOn();
        snprintf(output, output_size, "LED ON brightness %d", brigntness);  // 执行结果
        return true;

    } else if (strcmp(action, "off")==0) {
        led->TurnOff();
        snprintf(output, output_size, "LED OFF");  // 执行结果
        return true;

    } else if (strcmp(action, "blink")==0) {
        uint8_t duration = inDoc["value"].as<uint8_t>();
        if (duration < 1) duration = 1;
        if (duration > 60) duration = 60;
        led->Blink(-1, duration*1000);

        snprintf(output, output_size, "LED BLINK duration %ds", duration);  // 执行结果
        return true;
    }

    snprintf(output, output_size, "Error: invalid action.");  // 不能识别的动作
    return false;
}
//=================== Led Tool =============================

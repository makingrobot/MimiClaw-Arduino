#include "config.h"
#if BOARD_ESP32_S3_DEVKIT == 1

#include "driver/gpio.h"
#include "board_config.h"
#include "esp32_s3_board.h"
#include "src/framework/led/gpio_led.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "src/mimiclaw/arduino_json_psram.h"

#if CONFIS_USE_SPIFFS==1
#include <SPIFFS.h>
#elif CONFIG_USE_SDFS==1
#include <SPI.h>
#include <SD.h>
#endif

#define TAG "Esp32S3Board"

/**
 * Turn LED on/off
 * Input JSON: {"action", "on", "value": (0-255) }
 * Input JSON: {"action", "off" }
 */
bool tool_led_execute(const char *in_json, char *out, size_t len);

/**
 * Write a GPIO pin HIGH or LOW.
 * Input JSON: {"pin": <int>, "state": <0|1>}
 */
bool tool_gpio_write_execute(const char *input_json, char *output, size_t output_size);

/**
 * Read a single GPIO pin state.
 * Input JSON: {"pin": <int>}
 */
bool tool_gpio_read_execute(const char *input_json, char *output, size_t output_size);

/**
 * Read all allowed GPIO pin states at once.
 * Input JSON: {} (no parameters)
 */
bool tool_gpio_read_all_execute(const char *input_json, char *output, size_t output_size);

static const char LED_SKILL[] PROGMEM =
    "# LED Control\n"
    "Control led on/off and blink.\n"
    "\n"
    "## When to use\n"
    "When the user asks to led turn on/off.\n"
    "When the user asks to led blink.\n"
    "\n"
    "## How to use\n"
    "1. To **led on**: use led on brigntness n(0-255)\n"
    "2. To **led off**: use led off\n"
    "3. To **led blink**: use led blink duration n(1-60)\n"
    "\n"
    "## Example\n"
    "User: \"led on 255\"\n"
    "→ led { \"action\": \"on\", \"brigntness\": 255 }"
    "\n"
    "User: \"led off\"\n"
    "→ led { \"action\": \"off\" }\n"
    "\n"
    "User: \"led blink 2\"\n"
    "→ led { \"action\": \"blink\", \"duration\": 2 }";

static const char GPIO_SKILL[] PROGMEM =
    "# GPIO Control\n"
    "Control and monitor GPIO pins on the ESP32-S3 for digital I/O.\n"
    "\n"
    "## When to use\n"
    "When the user asks to:\n"
    "- Turn on/off LEDs, relays, or other outputs\n"
    "- Check switch states, button presses, or sensor readings\n"
    "- Confirm digital I/O status (switch confirmation)\n"
    "- Get an overview of all GPIO pin states\n"
    "\n"
    "## How to use\n"
    "1. To **read a switch/sensor**: use gpio_read with the pin number\n"
    "   - Returns HIGH (1) or LOW (0)\n"
    "   - HIGH typically means switch is ON / circuit closed\n"
    "   - LOW typically means switch is OFF / circuit open\n"
    "2. To **set an output**: use gpio_write with pin and state (1=HIGH, 0=LOW)\n"
    "3. To **scan all pins**: use gpio_read_all for a full status overview\n"
    "4. For **switch confirmation**: read the pin, report state, optionally toggle and re-read to verify\n"
    "\n"
    "## Pin safety\n"
    "- Only pins within the allowed range can be accessed\n"
    "- ESP32 flash pins (6-11) are always blocked\n"
    "- If a pin is rejected, suggest an alternative within the allowed range\n"
    "\n"
    "## Example\n"
    "User: \"Check if the switch on pin 4 is on\"\n"
    "→ gpio_read {\"pin\": 4}\n"
    "→ \"Pin 4 = HIGH\"\n"
    "→ \"The switch on pin 4 is currently ON (HIGH).\"\n"
    "\n"
    "User: \"Turn on the relay on pin 5\"\n"
    "→ gpio_write {\"pin\": 5, \"state\": 1}\n"
    "→ \"Pin 5 set to HIGH\"\n"
    "→ gpio_read {\"pin\": 5}\n"
    "→ \"Pin 5 = HIGH\"\n"
    "→ \"Relay on pin 5 is now ON. Confirmed HIGH.\"\n";

std::vector<uint8_t> Esp32S3Board::allow_pins = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 
        10, 11, 12, 13, 14, 15, 16, 17, 18,
        21, 
        35, 36, 37, 38, 39, 
        40, 41, 42, 45, 46, 47, 48
    };

void* create_board() { 
    return new Esp32S3Board();
}

Esp32S3Board::Esp32S3Board() : WifiBoard() {
    Log::Info(TAG, "===== Create Board ...... =====");

    Log::Info(TAG, "initial led.");
    led_ = new GpioLed(LED_PIN, true); //pwm.

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
        .execute = tool_led_execute,
    };
    _tools.push_back(&led);

     static const MimiTool gpio_write = {
        .name = "gpio_write",
        .description = "Set a GPIO pin HIGH or LOW. Controls LEDs, relays, and other digital outputs.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"pin\":{\"type\":\"integer\",\"description\":\"GPIO pin number\"},"
            "\"state\":{\"type\":\"integer\",\"description\":\"1 for HIGH, 0 for LOW\"}},"
            "\"required\":[\"pin\",\"state\"]}",
        .execute = tool_gpio_write_execute,
    };
    _tools.push_back(&gpio_write);

    static const MimiTool gpio_read = {
        .name = "gpio_read",
        .description = "Read a GPIO pin state. Returns HIGH or LOW. Use for checking switches, sensors, and digital inputs.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{\"pin\":{\"type\":\"integer\",\"description\":\"GPIO pin number\"}},"
            "\"required\":[\"pin\"]}",
        .execute = tool_gpio_read_execute,
    };
    _tools.push_back(&gpio_read);

    static const MimiTool gpio_readall = {
        .name = "gpio_read_all",
        .description = "Read all allowed GPIO pin states in a single call. Returns each pin's HIGH/LOW state.",
        .input_schema_json =
            "{\"type\":\"object\","
            "\"properties\":{},"
            "\"required\":[]}",
        .execute = tool_gpio_read_all_execute,
    };
    _tools.push_back(&gpio_readall);

    // 技能
    static const SkillInfo led_skill = {"led-control", LED_SKILL};
    _skills.push_back(&led_skill);

    static const SkillInfo gpio_skill = {"gpio-control", GPIO_SKILL};
    _skills.push_back(&gpio_skill);
}

void Esp32S3Board::InitFileSystem() {
    Log::Info( TAG, "Init file system ......" );

#if CONFIS_USE_SPIFFS==1
    if (!SPIFFS.begin()) {
        Log::Error(TAG, "SPIFFS Mount Failed");
        return;
    }

    filesystem_ = new FileSystem(SPIFFS);
    filesystem_->SetTotalBytes(SPIFFS.totalBytes());
    filesystem_->SetFreeBytes(SPIFFS.totalBytes() - SPIFFS.usedBytes());
    filesystem_->SetType("SPIFFS");

    filesystem_->DeleteFile("/spiffs/skills/weather.md");
    filesystem_->DeleteFile("spiffs/skills/skill-creator.md");
    filesystem_->DeleteFile("spiffs/sessions/tg_cli.jsonl");
    filesystem_->DeleteFile("spiffs/skills/led-control.md");
    filesystem_->DeleteFile("spiffs/skills/gpio-control.md");

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


//=================== Gpio Tool =============================
bool tool_gpio_write_execute(const char *input_json, char *output, size_t output_size)
{
    JsonDocument inDoc(&spiram_allocator);
    if (deserializeJson(inDoc, input_json)) {
        snprintf(output, output_size, "Error: invalid JSON input");
        return false;
    }

    if (inDoc["pin"].is<uint8_t>()) {
        snprintf(output, output_size, "Error: 'pin' required (integer)");
        return false;
    }
    if (inDoc["state"].is<uint8_t>()) {
        snprintf(output, output_size, "Error: 'state' required (0 or 1)");
        return false;
    }

    int pin = inDoc["pin"].as<uint8_t>();
    int state = inDoc["state"].as<uint8_t>();

    if (std::find(Esp32S3Board::allow_pins.begin(), Esp32S3Board::allow_pins.end(), pin)==Esp32S3Board::allow_pins.end()) {
        snprintf(output, output_size, "Error: pin %d is not in allowed list", pin);
        return false;
    }

    if (gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT) != ESP_OK) {
        snprintf(output, output_size, "Error: failed to configure/write pin %d", pin);
        return false;
    }

    gpio_set_level((gpio_num_t)pin, state ? 1 : 0);

    snprintf(output, output_size, "Pin %d set to %s", pin, state ? "HIGH" : "LOW");
    ESP_LOGI(TAG, "gpio_write: pin %d -> %s", pin, state ? "HIGH" : "LOW");

    return true;
}

bool tool_gpio_read_execute(const char *input_json, char *output, size_t output_size)
{
    JsonDocument inDoc(&spiram_allocator);
    if (deserializeJson(inDoc, input_json)) {
        snprintf(output, output_size, "Error: invalid JSON input");
        return false; 
    }

    if (inDoc["pin"].is<int>()) {
        snprintf(output, output_size, "Error: 'pin' required (integer)");
        return false; 
    }

    int pin = inDoc["pin"].as<int>();

    if (std::find(Esp32S3Board::allow_pins.begin(), Esp32S3Board::allow_pins.end(), pin)==Esp32S3Board::allow_pins.end()) {
        snprintf(output, output_size, "Error: pin %d is not in allowed list", pin);
        return false; 
    }

    /* read gpio value */
    if (gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT) != ESP_OK) {
        snprintf(output, output_size, "Error: failed to configure/read pin %d", pin);
        return false;
    }

    int level = gpio_get_level((gpio_num_t)pin);

    snprintf(output, output_size, "Pin %d = %s", pin, level ? "HIGH" : "LOW");
    ESP_LOGI(TAG, "gpio_read: pin %d = %s", pin, level ? "HIGH" : "LOW");

    return false;
}

bool tool_gpio_read_all_execute(const char *input_json, char *output, size_t output_size)
{
    (void)input_json;

    char *cursor = output;
    size_t remaining = output_size;
    int written;
    int count = 0;

    written = snprintf(cursor, remaining, "GPIO states: \n");
    if (written < 0 || (size_t)written >= remaining) {
        output[0] = '\0';
        return false;
    }
    cursor += (size_t)written;
    remaining -= (size_t)written;

    for (int pin : Esp32S3Board::allow_pins) {
        String str;
        if (gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT) == ESP_OK) {
            int level = gpio_get_level((gpio_num_t)pin);
            str = level ? "HIGH" : "LOW";
        } else {
            str = "N/A";
        }

        written = snprintf(cursor, remaining, "%d=%s\n", pin, str);
        if (written < 0 || (size_t)written >= remaining) break;
        cursor += (size_t)written;
        remaining -= (size_t)written;
        count++;
    }

    if (count == 0) {
        snprintf(output, output_size, "Error: no allowed GPIO pins configured");
        return false; 
    }

    ESP_LOGI(TAG, "gpio_read_all: %d pins read", count);
    return true;
}
//=================== Gpio Tool =============================

#endif
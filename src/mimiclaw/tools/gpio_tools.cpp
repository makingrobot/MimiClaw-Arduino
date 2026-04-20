
#include "gpio_tools.h"
#include "driver/gpio.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include "src/mimiclaw/arduino_json_psram.h"
#include "../mimi_board.h"

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

    if (std::find(MimiBoard::allow_pins.begin(), MimiBoard::allow_pins.end(), pin)==MimiBoard::allow_pins.end()) {
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

    if (std::find(MimiBoard::allow_pins.begin(), MimiBoard::allow_pins.end(), pin)==MimiBoard::allow_pins.end()) {
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

    for (int pin : MimiBoard::allow_pins) {
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
/*
 * MimiClaw-Arduino - Gpio tools
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _GPIO_TOOLS_H
#define _GPIO_TOOLS_H

#include <Arduino.h>

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

#endif //_GPIO_TOOLS_H
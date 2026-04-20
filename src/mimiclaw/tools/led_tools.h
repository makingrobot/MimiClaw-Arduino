/*
 * MimiClaw-Arduino - Led tools
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _LED_TOOLS_H
#define _LED_TOOLS_H

#include <Arduino.h>

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

/**
 * Turn LED on/off/blink
 * Input JSON: {"action", "on", "value": n(0-255) }
 * Input JSON: {"action", "off" }
 * Input JSON: {"action", "blink", "value": n(1-10) }
 */
bool tool_led_execute(const char *in_json, char *out, size_t len);

#endif



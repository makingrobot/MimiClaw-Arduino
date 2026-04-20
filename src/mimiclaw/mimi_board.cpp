
#include "mimi_board.h"

#include "mimi_tools.h"
#include "mimi_skills.h"
#include "tools/gpio_tools.h"
#include "tools/led_tools.h"

MimiBoard::MimiBoard() : Board() {
    
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



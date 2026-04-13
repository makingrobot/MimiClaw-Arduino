/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _TYPES_H
#define _TYPES_H

#include <functional>

typedef std::function<void(void)> callback_function_t;

enum ButtonAction {
    Click,
    DoubleClick,
    LongPress,
};

struct TouchPoint_t {
    int num = 0;
    int x = -1;
    int y = -1;
};

#endif //_TYPES_H
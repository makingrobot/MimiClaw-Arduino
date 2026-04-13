/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _EVENT_HANDLER_H
#define _EVENT_HANDLER_H

#include <cstdint>
#include <deque>
#include "../types.h"

class EventHandler {
public:
    static const int kEventError            = 1 << 0;
    static const int kEventScheduleTask     = 1 << 1;  
    static const int kEventNewVersion       = 1 << 2;

    EventHandler() { }
    ~EventHandler() { }
    
    virtual uint32_t GetEventBits() {
        return kEventError | kEventScheduleTask | kEventNewVersion;
    }

    virtual bool HandleEvent(uint32_t event_bits);

};

#endif //_EVENT_HANDLER_H
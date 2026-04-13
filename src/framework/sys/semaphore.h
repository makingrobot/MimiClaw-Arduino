/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

class Semaphore {
public:
    virtual bool Wait(int timeout_ms) = 0;
    virtual void Notify() = 0;
};

#endif
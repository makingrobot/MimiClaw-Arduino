/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _FRT_MUTEX_H
#define _FRT_MUTEX_H

#include <freertos/FreeRTOS.h>
#include <FreeRTOS/semphr.h>
#include "../mutex.h"

class FrtMutex :  public Mutex {
public:
    FrtMutex() 
    {
        mutex_semaphore_ = xSemaphoreCreateMutex(); // 创建互斥信号量
    }

    ~FrtMutex()
    {
        vSemaphoreDelete(mutex_semaphore_);
    }

    bool Lock(int timeout_ms = 0) override 
    {
        TickType_t tick = timeout_ms>=0 ? pdMS_TO_TICKS(timeout_ms) : portMAX_DELAY;
        return xSemaphoreTake(mutex_semaphore_, tick) == pdTRUE;
    }

    void Unlock() override 
    {
        xSemaphoreGive(mutex_semaphore_);
    }

private:
    SemaphoreHandle_t mutex_semaphore_ = NULL;
};

#endif  //_FRT_MUTEX_H
/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _FRT_SEMAPHORE_H
#define _FRT_SEMAPHORE_H

#include <freertos/FreeRTOS.h>
#include <FreeRTOS/semphr.h>
#include "../semaphore"

class FrtSemaphore :  public Semaphore {
public:
    FrtSemaphore() 
    {
        binary_semaphore_ = xSemaphoreCreateBinary(); // 创建二值信号量
    }

    ~FrtSemaphore()
    {
        vSemaphoreDelete(binary_semaphore_);
    }

    bool Wait(int timeout_ms = 0) override 
    {
        TickType_t tick = timeout_ms>=0 ? pdMS_TO_TICKS(timeout_ms) : portMAX_DELAY;
        return xSemaphoreTake(binary_semaphore_, tick) == pdTRUE;
    }

    void Notify() override 
    {
        xSemaphoreGive(binary_semaphore_);
    }

private:
    SemaphoreHandle_t binary_semaphore_ = NULL;
};

#endif  //_FRT_SEMAPHORE_H
/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _FRT_EVENTGROUP_H
#define _FRT_EVENTGROUP_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "log.h"

#define TAG "FrtEventGroup"

class FrtEventGroup {
public:
    FrtEventGroup(const std::string &name) : name_(name)
    {
        eventgroup_ = xEventGroupCreate();
    }

    ~FrtEventGroup()
    {
        vEventGroupDelete(eventgroup_);
    }

    /**
     * 发送到队列
     */
    void SetBits(uint32_t bits) 
    {
        xEventGroupSetBits(eventgroup_, bits);
    }

    /**
     * 从队列接收
     */
    uint32_t WaitBits(uint32_t event_bits, bool clear_on_exits, bool wait_for_all, int timeout_ms = 0) 
    {
        TickType_t ticksToWait = timeout_ms >= 0 ? pdMS_TO_TICKS(timeout_ms) : portMAX_DELAY;
        // 等待事件位被设置
        auto bits = xEventGroupWaitBits(eventgroup_, 
            event_bits,
            clear_on_exits ? pdTRUE : pdFALSE, /* 自动清除，避免重复响应 */
            wait_for_all ? pdTRUE : pdFALSE, /* 任一事件位被设置就返回 */
            ticksToWait /* 等待Ticks */
        );

        return bits;
    }

private:
    const std::string name_;
    EventGroupHandle_t eventgroup_;

};

#endif  //_FRT_EVENTGROUP_H
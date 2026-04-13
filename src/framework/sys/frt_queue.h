/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _FRT_QUEUE_H
#define _FRT_QUEUE_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "log.h"

#define TAG "FrtQueue"

class FrtQueue {
public:
    FrtQueue(const std::string &name, uint16_t length, uint16_t item_size) : name_(name)
    {
        queue_ = xQueueCreate(length, item_size);
    }

    ~FrtQueue()
    {
    }

    /**
     * 发送到队列
     */
    bool Send(const void *item, int timeout_ms = 0) 
    {
        TickType_t ticksToWait = timeout_ms >= 0 ? pdMS_TO_TICKS(timeout_ms) : portMAX_DELAY;
        if (xQueueSend(queue_, item, ticksToWait) != pdPASS) {
            Log::Warn(TAG, "发送数据到队列 %s 失败。", name_.c_str());
            return false;
        }
        return true;
    }

    /**
     * 覆盖写入队列，适用于队列长度为1的情况
     */
    bool Overwrite(const void *item) 
    {
        if (xQueueOverwrite(queue_, item) != pdPASS) {
            Log::Warn(TAG, "发送数据到队列 %s 失败。", name_.c_str());
            return false;
        }
        return true;
    }

    /**
     * 从队列接收
     */
    bool Receive(void *item, int timeout_ms = 0) {
        TickType_t ticksToWait = timeout_ms >= 0 ? pdMS_TO_TICKS(timeout_ms) : portMAX_DELAY;
        if (xQueueReceive(queue_, item, ticksToWait) != pdPASS) {
            Log::Warn(TAG, "从队列 %s 接收数据失败。", name_.c_str());
            return false;
        }
        return true;
    }

private:
    const std::string name_;
    QueueHandle_t queue_;

};

#endif  //_FRT_QUEUE_H
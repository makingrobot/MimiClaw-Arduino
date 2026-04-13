/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _FRT_TASK_H
#define _FRT_TASK_H

#include <functional>
#include <string>
#include <freertos/FreeRTOS.h>
#include "log.h"

#define TAG "FrtTask"

class FrtTask {
public:
    FrtTask(const std::string& name) : name_(name) {}

    const std::string& name() const { return name_; }

    const int state() const { return state_; }

    /**
     * 任务初始化函数
     */
    void OnInit(std::function<void()> init_function) { 
        init_function_ = init_function; 
    }

    void OnInit(std::function<void(void*)> init_param_function, void* parameter) { 
        init_param_function_ = init_param_function; 
        init_parameter_ = parameter;
    }

    /**
     * 任务循环体函数
     */
    void OnLoop(std::function<void()> loop_function) { 
        loop_function_ = loop_function; 
    }

    void OnLoop(std::function<void(void*)> loop_param_function, void* parameter) { 
        loop_param_function_ = loop_param_function; 
        loop_parameter_ = parameter;
    }

    /**
     * 挂起任务
     */
    void Suspend() {
        vTaskSuspend(task_handle_);
    }

    /**
     * 恢复任务执行
     */
    void Resume() {
        vTaskResume(task_handle_);
    }

    /**
     * 设置优先级
     */
    void SetPriority(uint8_t priority) {
        vTaskPrioritySet(task_handle_, priority);
    }

    /**
     * 启动任务
     */
    int Start(uint32_t stack_depth, uint8_t priority) {
        state_ = true;
        return xTaskCreate(
                    [](void *parameter) {
                        FrtTask *task = (FrtTask *)parameter;
                        Log::Info(TAG, "%s running on core %d", task->name().c_str(), xPortGetCoreID());
                        
                        // 优先调用有参数的Init函数
                        if (task->init_param_function_!=nullptr) {
                            task->init_param_function_(task->init_parameter_);
                        } else if (task->init_function_!=nullptr) {
                            task->init_function_();
                        }

                        // 优先调用有参数的Loop函数
                        if (task->loop_param_function_!=nullptr) {
                            while (task->state()) {
                                task->loop_param_function_(task->loop_parameter_);
                            }
                        } else if (task->loop_function_!=nullptr) {
                            while (task->state()) {
                                task->loop_function_();
                            }
                        }

                        vTaskDelete(NULL);
                    },                      /* 任务函数 TaskFunction_t */
                    name_.c_str(),          /* 任务名称 const char* */
                    stack_depth,            /* 堆栈字数 */
                    this,                   /* 自定义参数指针 void* */
                    priority,               /* 任务优先级 UBaseType_t */
                    &task_handle_          /* 任务句柄指针 TaskHandle_t* */
                );
    }

    void Stop() {
        state_ = false;
    }

private:
    const std::string name_;
    
    std::function<void()> init_function_ = nullptr;
    std::function<void(void*)> init_param_function_ = nullptr;
    void* init_parameter_;

    std::function<void()> loop_function_ = nullptr;
    std::function<void(void*)> loop_param_function_ = nullptr;
    void* loop_parameter_;

    TaskHandle_t task_handle_;
    volatile int state_ = true;
    
};

#endif  //_FRT_TASK_H
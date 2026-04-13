/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_ASYNC_PIPE_H
#define _AUDIO_ASYNC_PIPE_H

#include <functional>
#include <string>
#include <deque>
#include <vector>
#include <set>
#include <memory>
#include <freertos/FreeRTOS.h>

#include "../sys/mutex.h"
#include "../sys/semaphore.h"

#include "audio_common.h"
#include "audio_input.h"
#include "audio_output.h"
#include "audio_filter.h"
#include "audio_listener.h"

/**
 * 音频异步管道
 */
class AudioAsyncPipe {
public:
    AudioAsyncPipe();

    virtual void Start(AudioInput *input,  AudioOutput *output);
    virtual void Stop();

    const std::string& last_error() const { return last_error_; }

    void AddFilter(int order, std::shared_ptr<AudioFilter> filter)
    {
        filter->SetOrder(order);
        filter_set_.insert(filter);
    }

    void SetPipeListener(std::function<void(PipeAction)> listener)
    {
        pipe_listener_ = listener;
    }

    void SetAudioListener(std::shared_ptr<AudioListener> listener)
    {
        audio_listener_ = listener;
    }

protected:
    void InputTask();
    void OutputTask();

private:
    bool AddToDataQueue(const sample_data_t &data);
    bool ReadFromDataQueue(sample_data_t &data);

    volatile bool running_;

    AudioInput *input_ = nullptr;
    AudioOutput *output_ = nullptr;

    TaskHandle_t audio_in_handle_;
    TaskHandle_t audio_out_handle_;
    
    std::deque<sample_data_t> audio_data_queue_; //音频数据队列
    Mutex *audio_data_mutex_; //操作音频数据队列时的锁
    Semaphore *audio_data_semaphore_; //音频数据信号量

    std::string last_error_ = "";

    std::set<std::shared_ptr<AudioFilter>> filter_set_;
    std::function<void(PipeAction)> pipe_listener_;
    std::shared_ptr<AudioListener> audio_listener_;
};

#endif // _AUDIO_PIPE_H

#endif 
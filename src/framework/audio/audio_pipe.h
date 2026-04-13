/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_PIPE_H
#define _AUDIO_PIPE_H

#include <functional>
#include <string>
#include <vector>
#include <set>
#include <memory>
#include <freertos/FreeRTOS.h>

#include "audio_common.h"
#include "audio_input.h"
#include "audio_output.h"
#include "audio_filter.h"
#include "audio_listener.h"

/**
 * 音频管道
 */
class AudioPipe {
public:
    AudioPipe() 
    { 
        audio_listener_ = std::make_shared<AudioListener>();
    }

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

    void SetAudioListener(std::shared_ptr<AudioListener> audio_listener)
    {
        audio_listener_ = audio_listener;
    }

protected:
    void Execute();
    
private:
    volatile bool running_;

    AudioInput *input_ = nullptr;
    AudioOutput *output_ = nullptr;

    TaskHandle_t task_handle_;
    
    std::string last_error_ = "";

    std::set<std::shared_ptr<AudioFilter>> filter_set_;
    std::function<void(PipeAction)> pipe_listener_;

    std::shared_ptr<AudioListener> audio_listener_;
};

#endif // _AUDIO_PIPE_H

#endif 
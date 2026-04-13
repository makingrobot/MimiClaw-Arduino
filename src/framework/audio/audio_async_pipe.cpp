/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO == 1

#include "audio_async_pipe.h"
#include "../sys/log.h"
#include "../sys/mutex/frt_mutex.h"
#include "../sys/semaphore/frt_semaphore.h"

#define TAG "AudioAsyncPipe"

AudioAsyncPipe::AudioAsyncPipe() 
{
    audio_data_mutex_ = new FrtMutex();
    audio_data_semaphore_ = new FrtSemaphore();
}

void AudioAsyncPipe::Start(AudioInput *input, AudioOutput *output)
{
    input_ = input;
    output_ = output;

    // 启动输入任务
    xTaskCreate(
        [](void *parameter)
        {
            Log::Info(TAG, "AudioIn task running on core %d", xPortGetCoreID());

            AudioAsyncPipe *pipe = (AudioAsyncPipe *)parameter;
            pipe->InputTask();

            vTaskDelete(NULL);
        },                    /* 任务函数 TaskFunction_t */
        "AudioIn_Task",          /* 任务名称 const char* */
        8192,                 /* 堆栈字数 */
        this,                 /* 自定义参数指针 void* */
        tskIDLE_PRIORITY + 1, /* 任务优先级 UBaseType_t */
        &audio_in_handle_         /* 任务句柄指针 TaskHandle_t* */
    );
    
    // 启动输出任务
    xTaskCreate(
        [](void *parameter)
        {
            Log::Info(TAG, "AudioOut task running on core %d", xPortGetCoreID());

            AudioAsyncPipe *pipe = (AudioAsyncPipe *)parameter;
            pipe->OutputTask();

            vTaskDelete(NULL);
        },                    /* 任务函数 TaskFunction_t */
        "AudioOut_Task",          /* 任务名称 const char* */
        8192,                 /* 堆栈字数 */
        this,                 /* 自定义参数指针 void* */
        tskIDLE_PRIORITY + 1, /* 任务优先级 UBaseType_t */
        &audio_out_handle_         /* 任务句柄指针 TaskHandle_t* */
    );

    running_ = true;
}

void AudioAsyncPipe::InputTask()
{
    // 初始化
    bool ret = input_->Init();
    if (!ret)
    {
        last_error_ = "audio input init fail.";
        Log::Error(TAG, last_error_.c_str());
        if (pipe_listener_) 
        {
            pipe_listener_(PipeAction::Error);
        }
        return;
    }

    input_->SetAudioListener(audio_listener_);

    if (pipe_listener_) 
    {
        pipe_listener_(PipeAction::Inited);
    }

    // 处理数据
    while (running_)
    {
        if (input_->isEOF()) break;

        if (pipe_listener_) 
        {
            pipe_listener_(PipeAction::Processing);
        }
    
        sample_data_t input_data;
        if (!input_->Handle(input_data)) 
        { // 无数据
            continue;
        }

        audio_listener_->OnDataInput(input_data);

        AddToDataQueue(input_data);
    }

    Log::Info(TAG, "audio input task end.");
    
}

void AudioAsyncPipe::OutputTask()
{
    // 初始化
    bool ret = output_->Init();
    if (!ret)
    {
        last_error_ = "audio output init fail.";
        Log::Error(TAG, last_error_.c_str());
        if (pipe_listener_) 
        {
            pipe_listener_(PipeAction::Error);
        }
        return;
    }

    output_->SetAudioListener(audio_listener_);

    while (running_)
    {
        if (audio_data_queue_.empty())
        { //队例无数据
            audio_data_semaphore_->Wait(1000); // 等待1s
            if (audio_data_queue_.empty()) 
            { //还是无数据
                continue;
            }
        }
        
        // 加锁后操作
        sample_data_t audio_data;
        if (!ReadFromDataQueue(audio_data))
        { //未读取到数据
            continue;
        }

        sample_data_t output_data = audio_data;

        // --- STEP2：数据加工，对数据进行处理，如变声，混音等
        if (!filter_set_.empty())
        {
            filter_data_t filter_data;
            filter_data.length = audio_data.length;
            for (int i=0; i<audio_data.length; i++)
            {
                filter_data.samples.push_back(audio_data.samples[i]);
            }

            bool success = true;
            for (const std::shared_ptr<AudioFilter>& item : filter_set_)
            {
                filter_data_t out_data;
                if (!item->DoFilter(filter_data, out_data)) 
                {
                    success = false;
                    break;
                }
                // move data.
                filter_data = {
                    .samples = std::move(out_data.samples),
                    .length = out_data.length
                };
            }

            if (success) 
            { // 所有过滤器执行成功
                output_data.samples = filter_data.samples.data();
                output_data.length = filter_data.length;
            }
            else
            {
                continue;
            }
        }

        audio_listener_->OnDataOutput(output_data);

        // 输出
        output_->WriteSamples(output_data);
    }

    Log::Info(TAG, "audio output task end.");
}

bool AudioAsyncPipe::AddToDataQueue(const sample_data_t &data)
{
    MutexGuard lock_guard(audio_data_mutex_, -1);
    if (lock_guard.IsLocked()) {
        // 送入队列
        audio_data_queue_.push_back(data); //数据复制
        audio_data_semaphore_->Notify(); //有数据了
        return true;
    }
    return false;
}

bool AudioAsyncPipe::ReadFromDataQueue(sample_data_t &data)
{
    MutexGuard lock_guard(audio_data_mutex_, -1);
    if (lock_guard.IsLocked()) 
    { //拿到锁
        if (audio_data_queue_.empty())
        { //空队列
            return false;
        }

        sample_data_t audio_data = std::move(audio_data_queue_.front());
        audio_data_queue_.pop_front();

        data.samples[0] = audio_data.samples[0];
        data.samples[1] = audio_data.samples[1];
        data.length = audio_data.length;
        return true;
    }
    return false;
}

void AudioAsyncPipe::Stop()
{
    running_ = false;
    delay(1000); // 等待1s

    output_->Close();
    input_->Close();
}

#endif
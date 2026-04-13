/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO == 1

#include "audio_pipe.h"
#include "../sys/log.h"

#define TAG "AudioPipe"

void AudioPipe::Start(AudioInput *input, AudioOutput *output)
{
    input_ = input;
    output_ = output;

    running_ = true;
    // 启动任务
    xTaskCreate(
        [](void *parameter)
        {
            Log::Info(TAG, "Pipe task running on core %d", xPortGetCoreID());

            AudioPipe *pipe = (AudioPipe *)parameter;
            pipe->Execute();

            vTaskDelete(NULL);
        },                    /* 任务函数 TaskFunction_t */
        "Pipe_Task",          /* 任务名称 const char* */
        8192,                 /* 堆栈字数 */
        this,                 /* 自定义参数指针 void* */
        tskIDLE_PRIORITY + 1, /* 任务优先级 UBaseType_t */
        &task_handle_         /* 任务句柄指针 TaskHandle_t* */
    );
}

void AudioPipe::Execute()
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

    audio_config_t audio_cfg = input_->audio_config();
    Log::Info(TAG, "Audio input config: rate: %d, bits: %d, channels: %d",
        audio_cfg.input_rate, audio_cfg.input_bits, audio_cfg.input_channels);
    output_->SetAudioConfig(audio_cfg);
    
    ret = output_->Init();
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

    input_->SetAudioListener(audio_listener_);
    output_->SetAudioListener(audio_listener_);

    if (pipe_listener_) 
    {
        Log::Info(TAG, "Pipe inited.");
        pipe_listener_(PipeAction::Inited);
    }

    // 处理数据
    // AudioInput使用Handle方法拉取数据；
    // AudioOutput使用WriteXxx方法推送数据；
    while (running_)
    {
        if (input_->isEOF()) 
        {
            Log::Info(TAG, "Input EOF.");
            break;
        }

        if (pipe_listener_) 
        {
            pipe_listener_(PipeAction::Processing);
        }
    
        //--- STEP1：数据输入，从输入端取到原始数据
        sample_data_t input_data;
        if (!input_->Handle(input_data))
        { // 无数据
            Log::Info(TAG, "no data read.");
            continue;
        }

        audio_listener_->OnDataInput(input_data);

        sample_data_t output_data = input_data;

        // --- STEP2：数据加工，对数据进行处理，如变声，混音等
        if (!filter_set_.empty())
        {
            filter_data_t filter_data;
            filter_data.length = input_data.length;
            for (int i=0; i<input_data.length; i++)
            {
                filter_data.samples.push_back(input_data.samples[i]);
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

        //--- STEP3：数据输出
        output_->WriteSamples(output_data);
    }

    if (pipe_listener_) 
    {
        Log::Info(TAG, "Pipe ended.");
        pipe_listener_(PipeAction::Ended);
    }

    Log::Info(TAG, "handle end.");
}

void AudioPipe::Stop()
{
    running_ = false;
    delay(1000); // 等待1s

    output_->Close();
    input_->Close();
}

#endif
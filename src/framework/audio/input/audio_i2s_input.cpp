/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO == 1

#include "audio_i2s_input.h"
#include "../../sys/log.h"

#define TAG "AudioI2sInput"

bool AudioI2sInput::Init()
{
    codec_->Init(audio_config_);

    // 获取实际的音频采样数据
    audio_config_t cfg = codec_->audio_config();
    Log::Info(TAG, "I2S input config: rate:%lu, bits:%lu, channels:%lu", 
                    (uint32_t)cfg.input_rate, (uint8_t)cfg.input_bits, (uint8_t)cfg.input_channels);

    // 1s的数据大小
    uint32_t onesec_size = ((uint32_t)cfg.input_rate * ((uint8_t)cfg.input_bits / 8) * (uint8_t)cfg.input_channels);
    
    // 时长对应的数据长度
    buff_len_ = onesec_size * samples_msec_ / 1000;
    samples_buf_ = (int16_t*)heap_caps_malloc(buff_len_, MALLOC_CAP_DEFAULT);
    if (samples_buf_==NULL)
    {
        Log::Error(TAG, "Failed to allocate I2S buffer with size %u", buff_len_);
        return false;
    }
    samples_len_ = buff_len_ / 2;

    Log::Info(TAG, "Allocate I2S buffer with size %u", buff_len_);
    return true;
}

bool AudioI2sInput::Handle(sample_data_t& data)
{
    uint32_t bytes_read = codec_->Read(samples_buf_, samples_len_);
    duration_msec_ += samples_msec_;

    if (bytes_read == 0)
    {
        return false;
    }
    
    data.length = samples_len_;
    data.samples = samples_buf_;

    return true;
}

bool AudioI2sInput::Close()
{
    if (samples_buf_!=NULL)
    {
        // heap_caps_free(samples_buf_);
        samples_buf_ = NULL;
    }
    return true;
}

bool AudioI2sInput::isEOF()
{
    return false;
}

#endif // CONFIG_USE_AUDIO
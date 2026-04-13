/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#include <cstring>
#include <driver/i2s_common.h>
#include <Arduino.h>

#include "audio_i2s_codec.h"
#include "../../sys/log.h"
#include "../../sys/settings.h"

#define TAG "AudioI2sCodec"

AudioI2sCodec::AudioI2sCodec()
{
    volume_control_ = new VolumeControl();
}

bool AudioI2sCodec::Init(const audio_config_t &config)
{
    Log::Info(TAG, "init...");
    config_ = config;
    
    Settings settings("audio", false);
    output_volume_ = settings.GetInt("output_volume", output_volume_);
    if (output_volume_ <= 0) {
        Log::Warn(TAG, "Output volume value (%d) is too small, setting to default (10)", output_volume_);
        output_volume_ = 10;
    }
    volume_control_->SetVolume(output_volume_);

    return true;
}

void AudioI2sCodec::Start() 
{
    Log::Info(TAG, "Start...");
    EnableInput(true);
    EnableOutput(true);
}

void AudioI2sCodec::SetOutputVolume(int volume) 
{
    AudioCodec::SetOutputVolume(volume);
    volume_control_->SetVolume(volume);
    Log::Info(TAG, "Set output volume to %d", output_volume_);
    
    Settings settings("audio", true);
    settings.SetInt("output_volume", output_volume_);
}

#endif //CONFIG_USE_AUDIO
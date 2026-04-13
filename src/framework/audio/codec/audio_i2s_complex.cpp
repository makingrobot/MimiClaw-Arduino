/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#include "audio_i2s_complex.h"

#include <cmath>
#include <cstring>
#include <Arduino.h>

#include "../../sys/log.h"

#define TAG "AudioI2sComplex"

AudioI2sComplex::AudioI2sComplex(gpio_num_t spk_bclk, gpio_num_t spk_ws, gpio_num_t spk_dout, 
            gpio_num_t mic_sck, gpio_num_t mic_ws, gpio_num_t mic_din, gpio_num_t mclk) 
{
    speaker_ = new AudioI2sSimplexSpeaker(spk_bclk, spk_ws, spk_dout, mclk); 
    mic_ = new AudioI2sSimplexMic(mic_sck, mic_ws, mic_din, mclk); 
}

AudioI2sComplex::~AudioI2sComplex() 
{
    speaker_ = nullptr;
    mic_ = nullptr;
}

bool AudioI2sComplex::Init(const audio_config_t &config)
{
    Log::Info(TAG, "init...");
    AudioI2sCodec::Init(config);

    speaker_->Init(config);
    mic_->Init(config);
  
    return true;
}

uint32_t AudioI2sComplex::Write(const int16_t* data, uint32_t samples) 
{    
    if (!output_enabled_) return 0;

    volume_control_->ApplyVolume(data, samples);
    return speaker_->Write(data, samples);
}

uint32_t AudioI2sComplex::Read(int16_t* dest, uint32_t samples) 
{
    if (!input_enabled_) return 0;
    
    return mic_->Read(dest, samples);
}

#endif //CONFIG_USE_AUDIO
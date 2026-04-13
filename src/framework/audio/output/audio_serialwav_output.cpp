/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1 && CONFIG_USE_FS==1

#include <Arduino.h>
#include "audio_serialwav_output.h"
#include "../encoder/wav_encoder.h"
#include "../../sys/log.h"

#define TAG "AudioSerialWavOutput"

AudioSerialWavOutput::AudioSerialWavOutput()
{
}
    
AudioSerialWavOutput::AudioSerialWavOutput(AudioEncoder *encoder) 
    : encoder_(encoder) 
{
}

AudioSerialWavOutput::~AudioSerialWavOutput() {
    Close();
}

bool AudioSerialWavOutput::Init() {
    Log::Info(TAG, "init...");
    
    if (encoder_ == nullptr) 
    {
        // 创建编码器
#if CONFIG_AUDIO_CODER_WAV==1
        encoder_ = new WavEncoder();
#else
#pragma message("Please set CONFIG_AUDIO_CODER_WAV=1 in config file when use WAV encode.")
#endif
    }

    if (!encoder_) 
    {
        Log::Error(TAG, "encoder is null.");
        return false;
    }

    bool ret = encoder_->Init();
    if (!ret) 
    {
        Log::Error(TAG, "encoder init fail.");
        return false;
    }

    uint16_t head_size = encoder_->GetHeaderSize();
    uint8_t header[head_size];
    encoder_->GetHeaderData(header, 0, config_);
    Serial.write(header, head_size);

    return true;
}

uint32_t AudioSerialWavOutput::WriteSamples(const sample_data_t &data) {

    // 编码处理
    sample_data_t enc_data;
    if (!encoder_->Encode(data, enc_data)) 
    {
        return 0;
    }

    for (int j=0; j<enc_data.length; j+=2)
    {
        for (int i=0; i<config_.output_channels; i++) 
        {
            int16_t *samples_data = enc_data.samples;
            if (config_.output_bits==8)
            {
                uint8_t l = samples_data[j+i] & 0xff;
                Serial.write(l);
            }
            else
            {
                uint8_t l = samples_data[j+i] & 0xff;
                uint8_t h = (samples_data[j+i] >> 8) & 0xff;
                Serial.write(l);
                Serial.write(h);
            }
        }
    }

    return data.length;
}

bool AudioSerialWavOutput::Close()
{
    return true;
}

#endif
/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_AUDIO_CODER_WAV==1

#include <Arduino.h>
#include "wav_encoder.h"
#include "wav_header.h"

WavEncoder::WavEncoder()
{

}

WavEncoder::~WavEncoder()
{

}

bool WavEncoder::Init()
{
    return true;
}

bool WavEncoder::Encode(const sample_data_t &data, sample_data_t &out_data)
{
    out_data.length = data.length;
    out_data.samples = data.samples;
    return true;
}

void WavEncoder::GetHeaderData(uint8_t *header, uint32_t data_len, audio_config_t config) const
{
    const pcm_wav_header_t wav_header = PCM_WAV_HEADER_DEFAULT(data_len, 
        (uint8_t)config.output_bits, (uint32_t)config.output_rate, (uint8_t)config.output_channels);

    memcpy(header, &wav_header, PCM_WAV_HEADER_SIZE);
}

#endif
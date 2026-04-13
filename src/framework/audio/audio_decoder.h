/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO == 1

#ifndef _AUDIO_DECODER_H
#define _AUDIO_DECODER_H

#include "audio_input.h"

/**
 * 音频解码器基类
 */
class AudioDecoder
{
public:
    virtual bool Init() { return false; }
    virtual bool Decode() = 0;

    bool isEOF() { return eof_; }
    uint16_t channels() const { return channels_; }
    uint32_t sampleRate() const { return sampleRate_; }
    uint16_t bitsPerSample() const { return bitsPerSample_; }

    void GetSamples(sample_data_t &data)
    {
        data.samples = samples_,
        data.length = samples_len_;
    }

protected:
    bool eof_ = false;
    int16_t samples_[2];
    const uint32_t samples_len_ = 2;

    uint16_t channels_ = CHANNELS_2;
    uint32_t sampleRate_ = SAMPLE_RATE_44K;
    uint16_t bitsPerSample_ = SAMPLE_BITS_16;
};

#endif // _AUDIO_DECODER_H

#endif // CONFIG_USE_AUDIO
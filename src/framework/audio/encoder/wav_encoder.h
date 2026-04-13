/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_AUDIO_CODER_WAV==1

#ifndef _WAV_DECODER_H
#define _WAV_DECODER_H

#include "../audio_encoder.h"
#include "../audio_source.h"

class WavEncoder : public AudioEncoder
{
public:
    WavEncoder();
    virtual ~WavEncoder();
    virtual bool Init() override;
    virtual bool Encode(const sample_data_t &data, sample_data_t &out_data) override;

    virtual uint16_t GetHeaderSize() const override { return 44; }
    virtual void GetHeaderData(uint8_t *data, uint32_t data_len, audio_config_t config) const override;

private:
    
};

#endif //_WAV_ENCODER_H

#endif
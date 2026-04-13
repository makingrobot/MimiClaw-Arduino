/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_I2S_CODEC_H
#define _AUDIO_I2S_CODEC_H

#include <driver/i2s_std.h>
#include "../audio_codec.h"
#include "volume_control.h"

class AudioI2sCodec : public AudioCodec {
public:
    AudioI2sCodec();
    
    virtual bool Init(const audio_config_t &config) override;
    virtual void Start() override;
    virtual uint32_t Read(int16_t* dest, uint32_t samples) = 0;
    virtual uint32_t Write(const int16_t* data, uint32_t samples) = 0;
    
    virtual void SetOutputVolume(int volume) override;

    i2s_data_bit_width_t GetDataBitWidth(sample_bits_t bits) 
    {
        if (bits == SAMPLE_BITS_32) return I2S_DATA_BIT_WIDTH_32BIT;
        else if (bits == SAMPLE_BITS_24) return I2S_DATA_BIT_WIDTH_24BIT;
        else return I2S_DATA_BIT_WIDTH_16BIT;
    }

    i2s_slot_mode_t GetChannel(channels_t ch) 
    {
        if (ch == CHANNELS_1) return I2S_SLOT_MODE_MONO;
        else if (ch == CHANNELS_2) return I2S_SLOT_MODE_STEREO;
    }

protected:
    audio_config_t config_;
    VolumeControl *volume_control_;
    
};

#endif // _AUDIO_I2S_CODEC_H

#endif //CONFIG_USE_AUDIO
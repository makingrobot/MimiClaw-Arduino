/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_CODEC_H
#define _AUDIO_CODEC_H

#include "audio_common.h"

class AudioCodec {
public:
    virtual bool Init(const audio_config_t &config) 
    {
        audio_config_ = config;
        return true;
    }
    
    virtual void Start() = 0;
    virtual uint32_t Read(int16_t* dest, uint32_t samples) = 0;
    virtual uint32_t Write(const int16_t* data, uint32_t samples) = 0;
    
    virtual void SetOutputVolume(int volume) 
    { 
        output_volume_ = volume; 
    }

    virtual void EnableInput(bool enable) { input_enabled_ = enable; }
    virtual void EnableOutput(bool enable) { output_enabled_ = enable; }
    bool input_enabled() const { return input_enabled_;  }
    bool output_enabled() const { return output_enabled_ ; }
    int output_volume() const { return output_volume_; }

    virtual audio_config_t audio_config() { return audio_config_; };

protected:
    audio_config_t audio_config_;
    int output_volume_ = 30;  // 1-100;
    bool input_enabled_ = false;
    bool output_enabled_ = false;

};

#endif // _AUDIO_CODEC_H

#endif //CONFIG_USE_AUDIO
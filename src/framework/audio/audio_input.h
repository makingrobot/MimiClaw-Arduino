/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_INPUT_H
#define _AUDIO_INPUT_H

#include <memory>
#include "audio_common.h"
#include "audio_listener.h"

/**
 * 音频输入基类
 */
class AudioInput {
public:
    virtual bool Init() { return false; }

    virtual bool Handle(sample_data_t &data) = 0;
    virtual bool Close() = 0;
    virtual bool isEOF() = 0;
    
    virtual void SetAudioConfig(const audio_config_t &config) { audio_config_ = config; }
    virtual audio_config_t audio_config() { return audio_config_; }
    
    virtual void SetAudioListener(std::shared_ptr<AudioListener> listener) {
        audio_listener_ = listener;
    }

protected:
    audio_config_t audio_config_ = {
        .input_rate = SAMPLE_RATE_44K,
        .input_bits = SAMPLE_BITS_16,
        .input_channels = CHANNELS_2,
    };

    std::shared_ptr<AudioListener> audio_listener_;

};

#endif // _AUDIO_INPUT_H

#endif //CONFIG_USE_AUDIO
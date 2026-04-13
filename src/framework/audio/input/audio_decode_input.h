/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_DECODE_INPUT_H
#define _AUDIO_DECODE_INPUT_H

#include <string>
#include "../audio_input.h"
#include "../audio_source.h"
#include "../audio_decoder.h"

/**
 * 音频解码输入
 */
class AudioDecodeInput : public AudioInput {
public:
    AudioDecodeInput(AudioSource *source, const std::string& in_format);
    virtual ~AudioDecodeInput();

    virtual bool Init() override;
    virtual bool Handle(sample_data_t& data) override;
    virtual bool Close() override;
    virtual bool isEOF() override;

    virtual void SetAudioListener(std::shared_ptr<AudioListener> listener) override {
        audio_listener_ = listener;
        source_->SetAudioListener(listener);
    }

private:
    AudioSource *source_;
    AudioDecoder *decoder_;
    const std::string in_format_;

};

#endif // _AUDIO_DECODE_INPUT_H

#endif
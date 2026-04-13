/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_BUFFER_OUTPUT_H
#define _AUDIO_BUFFER_OUTPUT_H

#include "../audio_output.h"

/**
 * 缓存输出
 */
class AudioBufferOutput : public AudioOutput
{
public:
    AudioBufferOutput(AudioOutput *dest, int buf_size);
    virtual ~AudioBufferOutput();

    virtual uint32_t WriteSamples(const sample_data_t &data) override;
    virtual bool Close() override;

    virtual void SetAudioConfig(const audio_config_t &config) override;
    
    virtual void SetAudioListener(std::shared_ptr<AudioListener> listener) override {
        audio_listener_ = listener;
        output->SetAudioListener(listener);
    }

protected:
    AudioOutput *output;
    int buffSize;
    int16_t *leftSample;
    int16_t *rightSample;
    int writePtr;
    int readPtr;
    bool filled;
};

#endif //_AUDIO_BUFFER_OUTPUT_H

#endif
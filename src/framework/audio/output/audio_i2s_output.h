/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_I2S_OUTPUT_H
#define _AUDIO_I2S_OUTPUT_H

#include <driver/i2s_std.h>
#include "../audio_output.h"
#include "../audio_codec.h"

/**
 * I2s设备输出
 */
class AudioI2sOutput : public AudioOutput {
public:
    AudioI2sOutput(AudioCodec *codec) : codec_(codec) {  }
    virtual ~AudioI2sOutput() { };

    virtual bool Init() override;
    virtual uint32_t WriteSamples(const sample_data_t &data) override;

private:
    AudioCodec *codec_;

};

#endif // _AUDIO_I2S_OUTPUT_H

#endif
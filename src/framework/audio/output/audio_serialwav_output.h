
/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_SERIALWAV_OUTPUT_H
#define _AUDIO_SERIALWAV_OUTPUT_H

#include <string>
#include "../audio_output.h"
#include "../audio_encoder.h"

/**
 * 编码输出
 */
class AudioSerialWavOutput : public AudioOutput {
public:
    AudioSerialWavOutput();
    AudioSerialWavOutput(AudioEncoder *encoder);
    virtual ~AudioSerialWavOutput();

    virtual bool Init() override;
    virtual uint32_t WriteSamples(const sample_data_t &data) override;
    virtual bool Close() override;

private:
    AudioEncoder *encoder_;

};

#endif // _AUDIO_SERIALWAV_OUTPUT_H

#endif
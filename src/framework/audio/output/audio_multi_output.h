/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_MULTI_OUTPUT_H
#define _AUDIO_MULTI_OUTPUT_H

#include <vector>
#include "../audio_output.h"

/**
 * 多路输出
 */
class AudioMultiOutput : public AudioOutput {
public:
    AudioMultiOutput(const std::vector<AudioOutput*> &output_list, bool async=false) 
        : output_list_(output_list), async_(async) {  }
    virtual ~AudioMultiOutput() {  }

    bool Init() override;
    uint32_t WriteSamples(const sample_data_t &data) override;
    bool Close() override;

private:
    std::vector<AudioOutput*> output_list_;
    bool async_;

};

#endif // _AUDIO_MULTI_OUTPUT_H

#endif
/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#include <vector>
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include "audio_i2s_output.h"
#include "../../sys/log.h"

#define TAG "AudioI2sOutput"

bool AudioI2sOutput::Init() 
{
    Log::Info(TAG, "init...");
    codec_->Init(config_);
    return true;
}

uint32_t AudioI2sOutput::WriteSamples(const sample_data_t &data)
{
    return codec_->Write(data.samples, data.length);
}

#endif //CONFIG_USE_AUDIO
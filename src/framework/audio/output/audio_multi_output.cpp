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
#include "audio_multi_output.h"
#include "../../sys/log.h"

#define TAG "AudioMultiOutput"

bool AudioMultiOutput::Init() 
{
    return false;
}

uint32_t AudioMultiOutput::WriteSamples(const sample_data_t &data)
{
    return 0;
}

bool AudioMultiOutput::Close() 
{
    return false;
}

#endif //CONFIG_USE_AUDIO
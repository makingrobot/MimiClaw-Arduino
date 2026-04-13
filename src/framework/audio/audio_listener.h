/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _AUDIO_LISTENER_H
#define _AUDIO_LISTENER_H

#include <Arduino.h>
#include "audio_common.h"
#include "../sys/log.h"

class AudioListener
{
public:
    virtual void OnDataInput(const sample_data_t &data) { }

    virtual void OnDataOutput(const sample_data_t &data) { }
    
    virtual void OnStatus(const char* tag, int code, const char *text) 
    { 
        Log::Info(tag, "Code: %d, Message: %s", code, text);
    }

    virtual void OnMetadata(const char* tag, const char *type, const char *text) { }
};

#endif
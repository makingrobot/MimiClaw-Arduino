/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#include <Arduino.h>
#include "audio_progmem_source.h"
#include "../../sys/log.h"

#define TAG "AudioFileStreamSource"

AudioProgMemSource::AudioProgMemSource(const uint8_t *data, uint32_t len) 
    : progmemData(data), progmemLen(len)
{
    memPointer = 0;
}
    
AudioProgMemSource::~AudioProgMemSource()
{

}

uint32_t AudioProgMemSource::Read(uint8_t *data, uint32_t len)
{
    if (memPointer >= progmemLen) return 0;

    uint32_t toRead = progmemLen - memPointer;
    if (toRead > len) toRead = len;

    memcpy_P(data, progmemData+memPointer, toRead);
    memPointer += toRead;
    return toRead;
}

bool AudioProgMemSource::Seek(int32_t pos, int dir)
{
    uint32_t newPtr;
    switch (dir) {
        case SEEK_SET: newPtr = pos; break;
        case SEEK_CUR: newPtr = memPointer + pos; break;
        case SEEK_END: newPtr = progmemLen - pos; break;
        default: return false;
    }
    if (newPtr > progmemLen) return false;
    memPointer = newPtr;
    return true;
}

bool AudioProgMemSource::Close()
{
    progmemData = NULL;
    progmemLen = 0;
    memPointer = 0;
    return true;
}

#endif
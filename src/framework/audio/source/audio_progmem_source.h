/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_PROGMEM_SOURCE_H
#define _AUDIO_PROGMEM_SOURCE_H

#include <string>
#include "../audio_source.h"

/**
 * 音频内存数据输入
 */
class AudioProgMemSource : public AudioSource {
public:
    AudioProgMemSource(const uint8_t *data, uint32_t len);
    virtual ~AudioProgMemSource();

    virtual uint32_t Read(uint8_t *data, uint32_t len) override;
    virtual bool Seek(int32_t pos, int dir) override;
    virtual bool Close() override;

    virtual size_t GetPosition() override { return -1; }
    virtual size_t GetSize() override { return progmemLen; }

private:
    const uint8_t *progmemData;
    uint32_t progmemLen;
    uint32_t memPointer;
    
};

#endif // _AUDIO_PROGMEM_SOURCE_H

#endif //CONFIG_USE_AUDIO
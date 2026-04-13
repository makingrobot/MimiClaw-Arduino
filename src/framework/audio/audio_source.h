/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_SOURCE_H
#define _AUDIO_SOURCE_H

#include <memory>
#include "audio_listener.h"

/**
 * 音频源基类
 */
class AudioSource {
public:
    virtual uint32_t Read(uint8_t *data, uint32_t len) = 0;
    virtual bool Seek(int32_t pos, int dir) = 0;
    virtual bool Close() = 0;

    virtual bool Init() { return true; }
    virtual size_t GetPosition() { return 0; }
    virtual size_t GetSize() { return 0; }

    virtual void SetAudioListener(std::shared_ptr<AudioListener> listener) {
        audio_listener_ = listener;
    }

protected:
    std::shared_ptr<AudioListener> audio_listener_;

};

#endif // _AUDIO_SOURCE_H

#endif
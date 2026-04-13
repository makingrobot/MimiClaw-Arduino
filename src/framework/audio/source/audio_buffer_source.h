/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_BUFFER_SOURCE_H
#define _AUDIO_BUFFER_SOURCE_H

#include <string>
#include "../audio_source.h"

/**
 * 音频缓存输入
 */
class AudioBufferSource : public AudioSource {
public:
    AudioBufferSource(AudioSource *source, uint32_t buf_size);
    AudioBufferSource(AudioSource *source, void *buffer, uint32_t buf_size);  //Pre-allocated buffer by app
    virtual ~AudioBufferSource();

    virtual bool Init() override;
    virtual uint32_t Read(uint8_t *data, uint32_t len) override;
    virtual bool Seek(int32_t pos, int dir) override;
    virtual bool Close() override;

    virtual size_t GetPosition() override { return (source_) ? source_->GetPosition() : -1; }
    virtual size_t GetSize() override { return (source_) ? source_->GetSize() : -1; }

    virtual void SetAudioListener(std::shared_ptr<AudioListener> listener) override {
        audio_listener_ = listener;
        source_->SetAudioListener(listener);
    }

    enum { STATUS_FILLING=2, STATUS_UNDERFLOW };

private:
    void Fill();

    AudioSource *source_;
    uint32_t buffSize;

    uint8_t *buffer;
    bool deallocateBuffer;
    uint32_t writePtr;
    uint32_t readPtr;
    uint32_t length;
    bool filled;

};

#endif // _AUDIO_BUFFER_SOURCE_H

#endif //CONFIG_USE_AUDIO
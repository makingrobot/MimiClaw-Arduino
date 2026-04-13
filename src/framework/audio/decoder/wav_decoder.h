/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_AUDIO_CODER_WAV==1

#ifndef _WAV_DECODER_H
#define _WAV_DECODER_H

#include "../audio_decoder.h"
#include "../audio_source.h"

class WavDecoder : public AudioDecoder
{
public:
    WavDecoder(AudioSource *source, uint16_t buf_size);
    virtual ~WavDecoder();
    virtual bool Init() override;
    virtual bool Decode() override;
    void SetBufferSize(int sz) { buffSize = sz; }

private:
    bool ReadU32(uint32_t *dest) { return source_->Read(reinterpret_cast<uint8_t*>(dest), 4); }
    bool ReadU16(uint16_t *dest) { return source_->Read(reinterpret_cast<uint8_t*>(dest), 2); }
    bool ReadU8(uint8_t *dest) { return source_->Read(reinterpret_cast<uint8_t*>(dest), 1); }
    bool GetBufferedData(int bytes, void *dest);
    bool ReadWAVInfo();
    bool Stop();

    void Info(const char *name, int value);
    void Info(const char *name, const char *value);

    AudioSource *source_;

    uint32_t availBytes;

    // We need to buffer some data in-RAM to avoid doing 1000s of small reads
    uint32_t buffSize;
    uint8_t *buff;
    uint16_t buffPtr;
    uint16_t buffLen;

};

#endif //_WAV_DECODER_H

#endif
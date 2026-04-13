/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_AUDIO_CODER_MP3==1

#ifndef _HELIX_MP3_DECODER_H
#define _HELIX_MP3_DECODER_H

#include "../audio_decoder.h"
#include "../audio_source.h"
#include "src/libs/helix-mp3/mp3dec.h"

class HelixMP3Decoder : public AudioDecoder
{
public:
    HelixMP3Decoder(AudioSource *source);
    virtual ~HelixMP3Decoder();
    
    virtual bool Init() override;
    virtual bool Decode() override;

private:
    bool FillBufferWithValidFrame(); // Read until we get a valid syncword and min(feof, 2048) butes in the buffer
    
    // Helix MP3 decoder
    HMP3Decoder hMP3Decoder;

    // Input buffering
    uint8_t buff[1600]; // File buffer required to store at least a whole compressed frame
    int16_t buffValid;
    int16_t lastFrameEnd;

    // Output buffering
    int16_t outSample[1152 * 2]; // Interleaved L/R
    int16_t validSamples;
    int16_t curSample;

    AudioSource *source_;
};

#endif //_HELIX_MP3_DECODER_H

#endif
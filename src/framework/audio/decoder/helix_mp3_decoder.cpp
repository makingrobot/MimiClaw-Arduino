/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_AUDIO_CODER_MP3 == 1

#include <Arduino.h>
#include "helix_mp3_decoder.h"
#include "../../sys/log.h"

#define TAG "HelixMP3Decoder"

HelixMP3Decoder::HelixMP3Decoder(AudioSource *source) : source_(source)
{
}

HelixMP3Decoder::~HelixMP3Decoder()
{
    if (hMP3Decoder)
    {
        MP3FreeDecoder(hMP3Decoder);
    }
}

bool HelixMP3Decoder::Init()
{
    Log::Info(TAG, "init...");
    hMP3Decoder = MP3InitDecoder();
    if (!hMP3Decoder)
    {
        Log::Warn(TAG, PSTR("Out of memory error! hMP3Decoder==NULL\n"));
        return false;
    }
    // For sanity's sake...
    memset(buff, 0, sizeof(buff));
    memset(outSample, 0, sizeof(outSample));
    return true;
}

bool HelixMP3Decoder::Decode()
{
    if (eof_)
        return false;

    // If we've got data, try and pump it out...
    if (validSamples)
    {
        samples_[0] = outSample[curSample * 2];
        samples_[1] = outSample[curSample * 2 + 1];
        validSamples--;
        curSample++;
        
        return true;
    }

    // No samples available, need to decode a new frame
    if (FillBufferWithValidFrame())
    {
        // buff[0] start of frame, decode it...
        unsigned char *inBuff = reinterpret_cast<unsigned char *>(buff);
        int bytesLeft = buffValid;
        int ret = MP3Decode(hMP3Decoder, &inBuff, &bytesLeft, outSample, 0);
        if (ret)
        {
            Log::Error(TAG, "MP3 decode error %d", ret);
            // Error, skip the frame...
            // char buff[48];
            // sprintf(buff, "MP3 decode error %d", ret);
            // source_->Status()->StatusCB(ret, buff);
            return false;
        }

        lastFrameEnd = buffValid - bytesLeft;
        MP3FrameInfo fi;
        MP3GetLastFrameInfo(hMP3Decoder, &fi);
        if (fi.samprate > 0) sampleRate_ = fi.samprate;
        if (fi.nChans > 0) channels_ = fi.nChans;
        if (fi.bitsPerSample > 0) bitsPerSample_ = fi.bitsPerSample;

        curSample = 0;
        validSamples = fi.outputSamps / channels_;

        samples_[0] = outSample[curSample * 2];
        samples_[1] = outSample[curSample * 2 + 1];
        validSamples--;
        curSample++;
        
        return true;
    }

    return false;
}

bool HelixMP3Decoder::FillBufferWithValidFrame()
{
    buff[0] = 0; // Destroy any existing sync word @ 0
    int nextSync;
    do
    {
        nextSync = MP3FindSyncWord(buff + lastFrameEnd, buffValid - lastFrameEnd);
        if (nextSync >= 0)
            nextSync += lastFrameEnd;
        lastFrameEnd = 0;
        if (nextSync == -1)
        {
            if (buff[buffValid - 1] == 0xff)
            { // Could be 1st half of syncword, preserve it...
                buff[0] = 0xff;
                buffValid = source_->Read(buff + 1, sizeof(buff) - 1);
                if (buffValid == 0)
                {
                    eof_ = true;
                    return false; // No data available, EOF
                }
            }
            else
            { // Try a whole new buffer
                buffValid = source_->Read(buff, sizeof(buff));
                if (buffValid == 0)
                {
                    eof_ = true;
                    return false; // No data available, EOF
                }
            }
        }
    } while (nextSync == -1);

    // Move the frame to start at offset 0 in the buffer
    buffValid -= nextSync; // Throw out prior to nextSync
    memmove(buff, buff + nextSync, buffValid);

    // We have a sync word at 0 now, try and fill remainder of buffer
    buffValid += source_->Read(buff + buffValid, sizeof(buff) - buffValid);

    return true;
}

#endif
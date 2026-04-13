/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#include "audio_buffer_output.h"

AudioBufferOutput::AudioBufferOutput(AudioOutput *dest, int buff_size)
{
    buffSize = buff_size;
    leftSample = (int16_t*)malloc(sizeof(int16_t) * buffSize);
    rightSample = (int16_t*)malloc(sizeof(int16_t) * buffSize);
    writePtr = 0;
    readPtr = 0;
    output = dest;
}

AudioBufferOutput::~AudioBufferOutput()
{
    free(leftSample);
    free(rightSample);
}

void AudioBufferOutput::SetAudioConfig(const audio_config_t &config)
{
    output->SetAudioConfig(config);
}

bool AudioBufferOutput::Close()
{
    return output->Close();
}

uint32_t AudioBufferOutput::WriteSamples(const sample_data_t &data)
{
    // First, try and fill I2S...
    // if (filled) {
    //     while (readPtr != writePtr) {
    //     int16_t s[2] = {leftSample[readPtr], rightSample[readPtr]};
    //     if (!sink->ConsumeSample(s)) break; // Can't stuff any more in I2S...
    //         readPtr = (readPtr + 1) % buffSize;
    //     }
    // }

    // // Now, do we have space for a new sample?
    // int nextWritePtr = (writePtr + 1) % buffSize;
    // if (nextWritePtr == readPtr) {
    //     filled = true;
    //     return false;
    // }
    // leftSample[writePtr] = data[0];
    // rightSample[writePtr] = data[1];
    // writePtr = nextWritePtr;
    return data.length;
}

#endif
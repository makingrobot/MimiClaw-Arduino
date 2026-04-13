/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO == 1

#include <Arduino.h>
#include "audio_buffer_source.h"
#include "../../sys/log.h"

#define TAG "AudioBufferSource"

AudioBufferSource::AudioBufferSource(AudioSource *source, uint32_t buff_size)
    : source_(source), buffSize(buff_size)
{
    buffer = (uint8_t *)malloc(sizeof(uint8_t) * buffSize);
    if (!buffer)
    {
        Log::Warn(TAG, PSTR("Unable to allocate AudioBufferSource::buffer[]"));
    }
    deallocateBuffer = true;
    writePtr = 0;
    readPtr = 0;
    length = 0;
    filled = false;
}

AudioBufferSource::AudioBufferSource(AudioSource *source, void *inBuff, uint32_t buff_size)
    : source_(source), buffSize(buff_size)
{
    buffer = (uint8_t *)inBuff;
    deallocateBuffer = false;
    writePtr = 0;
    readPtr = 0;
    length = 0;
    filled = false;
}

AudioBufferSource::~AudioBufferSource()
{
    if (deallocateBuffer)
        free(buffer);
    buffer = NULL;
}

bool AudioBufferSource::Init()
{
    Log::Info(TAG, "init...");
    return source_->Init();
}

uint32_t AudioBufferSource::Read(uint8_t *data, uint32_t len)
{
    if (!buffer)
        return source_->Read(data, len);

    uint32_t bytes = 0;
    if (!filled)
    {
        // Fill up completely before returning any data at all
        audio_listener_->OnStatus(TAG, STATUS_FILLING, PSTR("Refilling buffer"));
        length = source_->Read(buffer, buffSize);
        writePtr = length % buffSize;
        filled = true;
    }

    // Pull from buffer until we've got none left or we've satisfied the request
    uint8_t *ptr = data;
    uint32_t toReadFromBuffer = (len < length) ? len : length;
    if ((toReadFromBuffer > 0) && (readPtr >= writePtr))
    {
        uint32_t toReadToEnd = (toReadFromBuffer < (uint32_t)(buffSize - readPtr)) ? toReadFromBuffer : (buffSize - readPtr);
        memcpy(ptr, &buffer[readPtr], toReadToEnd);
        readPtr = (readPtr + toReadToEnd) % buffSize;
        len -= toReadToEnd;
        length -= toReadToEnd;
        ptr += toReadToEnd;
        bytes += toReadToEnd;
        toReadFromBuffer -= toReadToEnd;
    }
    if (toReadFromBuffer > 0)
    { // We know RP < WP at this point
        memcpy(ptr, &buffer[readPtr], toReadFromBuffer);
        readPtr = (readPtr + toReadFromBuffer) % buffSize;
        len -= toReadFromBuffer;
        length -= toReadFromBuffer;
        ptr += toReadFromBuffer;
        bytes += toReadFromBuffer;
        toReadFromBuffer -= toReadFromBuffer;
    }

    if (len)
    {
        // Still need more, try direct read from src
        bytes += source_->Read(ptr, len);
        // We're out of buffered data, need to force a complete refill.  Thanks, @armSeb
        readPtr = 0;
        writePtr = 0;
        length = 0;
        filled = false;
        audio_listener_->OnStatus(TAG, STATUS_UNDERFLOW, PSTR("Buffer underflow"));
    }

    Fill();

    return bytes;
}

bool AudioBufferSource::Seek(int32_t pos, int dir)
{
    if (dir == SEEK_CUR && (readPtr + pos) < length)
    {
        readPtr += pos;
        return true;
    }
    else
    {
        // Invalidate
        readPtr = 0;
        writePtr = 0;
        length = 0;
        return source_->Seek(pos, dir);
    }
}

bool AudioBufferSource::Close()
{
    if (deallocateBuffer)
        free(buffer);
    buffer = NULL;
    return source_->Close();
}

void AudioBufferSource::Fill()
{
    if (!buffer)
        return;

    if (length < buffSize)
    {
        // Now try and opportunistically fill the buffer
        if (readPtr > writePtr)
        {
            if (readPtr == writePtr + 1)
                return;
            uint32_t bytesAvailMid = readPtr - writePtr - 1;
            int cnt = source_->Read(&buffer[writePtr], bytesAvailMid);
            length += cnt;
            writePtr = (writePtr + cnt) % buffSize;
            return;
        }

        if (buffSize > writePtr)
        {
            uint32_t bytesAvailEnd = buffSize - writePtr;
            int cnt = source_->Read(&buffer[writePtr], bytesAvailEnd);
            length += cnt;
            writePtr = (writePtr + cnt) % buffSize;
            if (cnt != (int)bytesAvailEnd)
                return;
        }

        if (readPtr > 1)
        {
            uint32_t bytesAvailStart = readPtr - 1;
            int cnt = source_->Read(&buffer[writePtr], bytesAvailStart);
            length += cnt;
            writePtr = (writePtr + cnt) % buffSize;
        }
    }
}

#endif
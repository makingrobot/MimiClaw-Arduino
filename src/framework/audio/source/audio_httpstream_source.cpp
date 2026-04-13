/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#include <Arduino.h>
#include "audio_httpstream_source.h"
#include "../../sys/log.h"

#define TAG "AudioHttpStreamSource"

AudioHttpStreamSource::AudioHttpStreamSource(const std::string &audio_url) : audio_url_(audio_url)
{
    retry_times_ = 0;
}

AudioHttpStreamSource::~AudioHttpStreamSource()
{
    http_.end();
}

bool AudioHttpStreamSource::Init() 
{
    Log::Info(TAG, "init...");
    return Open();
}

bool AudioHttpStreamSource::Open()
{
    position_ = 0;
    http_.begin(client_, audio_url_.c_str());
    http_.setReuse(true);
    int code = http_.GET();
    if (code != HTTP_CODE_OK) {
        http_.end();
        audio_listener_->OnStatus(TAG, STATUS_HTTPFAIL, PSTR("Can't open HTTP request"));
        Log::Warn(TAG, "Can't open url %s, status code: %d", audio_url_.c_str(), code);
        return false;
    }
    size_ = http_.getSize();
    return true;
}

uint32_t AudioHttpStreamSource::Read(uint8_t *data, uint32_t len)
{
retry_label:
    if (!http_.connected()) {
        audio_listener_->OnStatus(TAG, STATUS_DISCONNECTED, PSTR("Stream disconnected"));
        http_.end();

        for (int i = 0; i < retry_times_; i++) {
            char buff[64];
            sprintf_P(buff, PSTR("Attempting to reconnect, try %d"), i);
            audio_listener_->OnStatus(TAG, STATUS_RECONNECTING, buff);
            delay(retry_delayms_);
            if (Open()) {
                audio_listener_->OnStatus(TAG, STATUS_RECONNECTED, PSTR("Stream reconnected"));
                break;
            }
        }
        if (!http_.connected()) {
            audio_listener_->OnStatus(TAG, STATUS_DISCONNECTED, PSTR("Unable to reconnect"));
            return 0;
        }
    }

    if ((size_ > 0) && (position_ >= size_)) return 0;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    NetworkClient *stream = http_.getStreamPtr();
#else
    WiFiClient *stream = http_.getStreamPtr();
#endif

    // Can't read past EOF...
    if ( (size_ > 0) && (len > (uint32_t)(position_ - size_)) ) {
        len = position_ - size_;
    }

    int start = millis();
    while ((stream->available() < (int)len) && (millis() - start < 500)) {
        yield();
    }

    size_t avail = stream->available();
    if (!avail) {
        audio_listener_->OnStatus(TAG, STATUS_NODATA, PSTR("No stream data available"));
        http_.end();
        goto retry_label;
    }
    if (avail == 0) return 0;
    if (avail < len) len = avail;

    int bytesRead = stream->read(data, len);
    position_ += bytesRead;
    return bytesRead;
}

bool AudioHttpStreamSource::Seek(int32_t pos, int dir)
{
    Serial.printf_P(PSTR("ERROR! AudioHTTPStreamInput::seek not implemented!"));
    return false;
}

bool AudioHttpStreamSource::Close()
{
    http_.end();
    return true;
}

#endif
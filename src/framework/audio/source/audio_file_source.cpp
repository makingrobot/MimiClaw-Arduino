/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO == 1 && CONFIG_USE_FS == 1

#include "audio_file_source.h"
#include "../../sys/log.h"

#define TAG "AudioFileSource"

bool AudioFileSource::Init()
{
    Log::Info(TAG, "init...");
    file_ = fs_->OpenFile(filename_.c_str(), "r");
    if (!file_)
    {
        Log::Warn(TAG, "file %s open failed.", filename_);
        return false;
    }

    Log::Info(TAG, "file %s size: %d", file_.name(), file_.size());
    return true;
}

uint32_t AudioFileSource::Read(uint8_t *data, uint32_t len)
{
    size_t bytes_read = file_.read(data, len);
    Log::Debug(TAG, "read at pos: %d,  readed %d bytes.", file_.position(), bytes_read);
    return bytes_read;
}

bool AudioFileSource::Seek(int32_t pos, int dir)
{
    if (!file_)
        return false;

    if (dir == SEEK_SET)
        return file_.seek(pos);
    else if (dir == SEEK_CUR)
        return file_.seek(file_.position() + pos);
    else if (dir == SEEK_END)
        return file_.seek(file_.size() + pos);
    return false;
}

bool AudioFileSource::Close()
{
    if (!file_)
        return false;

    file_.close();
    return true;
}

#endif
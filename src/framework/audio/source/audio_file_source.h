/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1 && CONFIG_USE_FS==1

#ifndef _AUDIO_FILE_SOURCE_H
#define _AUDIO_FILE_SOURCE_H

#include <string>
#include <FS.h>
#include "../audio_source.h"
#include "../../file/file_system.h"

/**
 * 音频文件输入
 */
class AudioFileSource : public AudioSource {
public:
    AudioFileSource(FileSystem *fs, const std::string &filename) 
        : fs_(fs), filename_(filename) { }
    virtual ~AudioFileSource() { Close(); }

    virtual bool Init() override;
    virtual uint32_t Read(uint8_t *data, uint32_t len) override;
    virtual bool Seek(int32_t pos, int dir) override;
    virtual bool Close() override;

    virtual size_t GetPosition() override { return (file_) ? file_.position() : -1; }
    virtual size_t GetSize() override { return (file_) ? file_.size() : -1; }

private:
    FileSystem *fs_;
    const std::string filename_;
    fs::File file_;
    
};

#endif // _AUDIO_FILE_SOURCE_H

#endif //CONFIG_USE_AUDIO
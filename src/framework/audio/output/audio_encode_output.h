
/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1 && CONFIG_USE_FS==1

#ifndef _AUDIO_ENCODE_OUTPUT_H
#define _AUDIO_ENCODE_OUTPUT_H

#include <string>
#include "../audio_output.h"
#include "../audio_encoder.h"
#include "src/framework/file/file_system.h"

/**
 * 编码输出
 */
class AudioEncodeOutput : public AudioOutput {
public:
    AudioEncodeOutput(FileSystem *fsys, const std::string &filename, const std::string& out_format);
    virtual ~AudioEncodeOutput();

    virtual bool Init() override;
    virtual uint32_t WriteSamples(const sample_data_t &data) override;
    virtual bool Close() override;

private:
    AudioEncoder *encoder_;
    FileSystem *fsys_;

    const std::string filename_;
    fs::File file_;
    const std::string out_format_;

};

#endif // _AUDIO_ENCODE_OUTPUT_H

#endif
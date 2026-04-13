/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1 && CONFIG_USE_FS==1

#include <Arduino.h>
#include "audio_encode_output.h"
#include "../encoder/wav_encoder.h"
#include "../../sys/log.h"

#define TAG "AudioEncodeOutput"

AudioEncodeOutput::AudioEncodeOutput(FileSystem *fsys, const std::string &filename, const std::string& out_format) 
    : fsys_(fsys), filename_(filename), out_format_(out_format) {

}
    
AudioEncodeOutput::~AudioEncodeOutput() {
    Close();
}

bool AudioEncodeOutput::Init() {
    Log::Info(TAG, "init...");
    if (fsys_->ExistsFile(filename_.c_str()))
    { // 存在就删除。
        fsys_->DeleteFile(filename_.c_str());
        Log::Info(TAG, "File %s deleted.", filename_.c_str());
    }
    file_ = fsys_->OpenFile(filename_.c_str(), "w+");
    if (!file_)
    { // 以写入方式打开文件
        Log::Warn(TAG, "file %s open failed.", filename_);
        return false;
    }

    // 创建编码器
    if (out_format_ == "wav") {
#if CONFIG_AUDIO_CODER_WAV==1
        encoder_ = new WavEncoder();
#else
        #pragma message("Please set CONFIG_AUDIO_CODER_WAV=1 in config file when use WAV encode.")
#endif
    }
    
    if (!encoder_) {
        Log::Error(TAG, "encoder is null.");
        return false;
    }

    bool ret = encoder_->Init();
    if (!ret) {
        Log::Error(TAG, "encoder init fail.");
        return false;
    }

    uint16_t head_size = encoder_->GetHeaderSize();
    uint8_t header[head_size];
    memset(header, 0, head_size);
    file_.write(header, head_size);

    return true;
}

uint32_t AudioEncodeOutput::WriteSamples(const sample_data_t &data) {

    // 数据预处理
    // TODO：是否要等待数据量足够后才能编码？

    // 编码处理
    sample_data_t enc_data;
    if (!encoder_->Encode(data, enc_data))
    {
        return 0;
    }
    
    // 写入文件
    size_t len = file_.write((uint8_t*)(enc_data.samples), enc_data.length*2);
    return len;
}

bool AudioEncodeOutput::Close()
{
    if (!file_)
        return false;

    Log::Info(TAG, "encode config: format:%s, rate:%lu, bits:%lu, channels:%lu", 
                    out_format_.c_str(), (uint32_t)config_.output_rate, (uint8_t)config_.output_bits, (uint8_t)config_.output_channels);

    uint16_t head_size = encoder_->GetHeaderSize();
    uint8_t header[head_size];
    encoder_->GetHeaderData(header, file_.size()-head_size, config_);

    // Write real header out
    file_.seek(0, SeekSet);
    file_.write(header, head_size);
    file_.close();

    Log::Info(TAG, "Output %s size: %d", filename_.c_str(), file_.size());
    return true;
}

#endif
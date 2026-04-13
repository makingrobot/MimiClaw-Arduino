/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO == 1

#include <Arduino.h>
#include "audio_decode_input.h"
#include "../decoder/helix_mp3_decoder.h"
#include "../decoder/wav_decoder.h"
#include "../../sys/log.h"

#define TAG "AudioDecodeInput"

AudioDecodeInput::AudioDecodeInput(AudioSource *source, const std::string &in_format)
    : source_(source), in_format_(in_format)
{
}

AudioDecodeInput::~AudioDecodeInput()
{
    decoder_ = nullptr;
}

bool AudioDecodeInput::Init()
{
    Log::Info(TAG, "init...");
    bool ret = source_->Init();
    if (!ret)
    {
        Log::Error(TAG, "input source init fail.");
        return false;
    }

    // 创建解码器
    if (in_format_ == "mp3")
    {
#if CONFIG_AUDIO_CODER_MP3 == 1
        decoder_ = new HelixMP3Decoder(source_);
#else
#pragma message("Please set CONFIG_AUDIO_CODER_MP3=1 in config file when use MP3 decode.")
#endif
    }
    else if (in_format_ == "wav")
    {
#if CONFIG_AUDIO_CODER_WAV == 1
        decoder_ = new WavDecoder(source_, 2000);
#else
#pragma message("Please set CONFIG_AUDIO_CODER_WAV=1 in config file when use WAV decode.")
#endif
    }

    if (!decoder_)
    {
        Log::Error(TAG, "decoder is null.");
        return false;
    }

    ret = decoder_->Init();
    if (!ret)
    {
        Log::Error(TAG, "decoder init fail.");
        return false;
    }

    audio_config_ = {
        .input_rate = (sample_rate_t)decoder_->sampleRate(),
        .input_bits = (sample_bits_t)decoder_->bitsPerSample(),
        .input_channels = (channels_t)decoder_->channels(),
        .output_rate = (sample_rate_t)decoder_->sampleRate(),
        .output_bits = (sample_bits_t)decoder_->bitsPerSample(),
        .output_channels = (channels_t)decoder_->channels()
    };
    return true;
}

bool AudioDecodeInput::Handle(sample_data_t &data)
{
    if (!decoder_->Decode())
        return false;

    decoder_->GetSamples(data);
    return true;
}

bool AudioDecodeInput::isEOF()
{
    return decoder_->isEOF();
}

bool AudioDecodeInput::Close()
{
    return source_->Close();
}

#endif
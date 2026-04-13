/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_AUDIO_CODEC_ES8311==1

#ifndef _AUDIO_ES8311_CODEC_H
#define _AUDIO_ES8311_CODEC_H

#include <driver/i2s_std.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include "src/libs/esp_codec_dev/esp_codec_dev.h"
#include "src/libs/esp_codec_dev/esp_codec_dev_defaults.h"
#include "../audio_common.h"
#include "audio_i2s_codec.h"

class AudioEs8311Codec : public AudioI2sCodec {
public:
    AudioEs8311Codec(void* i2c_master_handle, i2c_port_t i2c_port, int input_sample_rate, int output_sample_rate,
        gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din,
        gpio_num_t pa_pin, uint8_t es8311_addr, bool use_mclk = true, bool pa_inverted = false);
    virtual ~AudioEs8311Codec();

    virtual bool Init(const audio_config_t &config) override;
    virtual uint32_t Read(int16_t* dest, uint32_t samples) override;
    virtual uint32_t Write(const int16_t* data, uint32_t samples) override;
    
    virtual void SetOutputVolume(int volume) override;
    virtual void EnableInput(bool enable) override;
    virtual void EnableOutput(bool enable) override;

private:
    const audio_codec_data_if_t* data_if_ = nullptr;
    const audio_codec_ctrl_if_t* ctrl_if_ = nullptr;
    const audio_codec_if_t* codec_if_ = nullptr;
    const audio_codec_gpio_if_t* gpio_if_ = nullptr;

    esp_codec_dev_handle_t dev_ = nullptr;
    gpio_num_t pa_pin_ = GPIO_NUM_NC;
    bool pa_inverted_ = false;

    bool input_reference_; // 是否使用参考输入，实现回声消除
    int input_channels_; // 输入通道数
    int input_sample_rate_;
    int output_sample_rate_;

    i2s_chan_handle_t tx_handle_ = nullptr;
    i2s_chan_handle_t rx_handle_ = nullptr;

    void CreateDuplexChannels(gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din);
    void UpdateDeviceState();

};

#endif // _AUDIO_ES8311_CODEC_H

#endif //CONFIG_USE_AUDIO
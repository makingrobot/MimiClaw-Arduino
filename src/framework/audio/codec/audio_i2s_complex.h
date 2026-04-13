/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_I2S_COMPLEX_H
#define _AUDIO_I2S_COMPLEX_H

#include "audio_i2s_codec.h"
#include "audio_i2s_simplex.h"

/**
 * I2S 麦克风 + 喇叭
 */
class AudioI2sComplex : public AudioI2sCodec {
public:
    // mic
    AudioI2sComplex(gpio_num_t spk_bclk, gpio_num_t spk_ws, gpio_num_t spk_dout, 
            gpio_num_t mic_sck, gpio_num_t mic_ws, gpio_num_t mic_din, gpio_num_t mclk=GPIO_NUM_NC);
    virtual ~AudioI2sComplex();

    virtual bool Init(const audio_config_t &config) override;
    virtual uint32_t Write(const int16_t* data, uint32_t samples) override;
    virtual uint32_t Read(int16_t* dest, uint32_t samples) override;

private:
    AudioI2sSimplex *speaker_;
    AudioI2sSimplex *mic_;

};

#endif //_AUDIO_I2S_COMPLEX_H

#endif
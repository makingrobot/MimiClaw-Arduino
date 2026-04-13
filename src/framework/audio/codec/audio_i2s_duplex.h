/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_I2S_DUPLEX_H
#define _AUDIO_I2S_DUPLEX_H

#include <driver/gpio.h>
#include "audio_i2s_codec.h"
#include "../../board/i2s_driver.h"

/**
 * 双工 I2S
 */
class AudioI2sDuplex : public AudioI2sCodec {
public:
    AudioI2sDuplex(gpio_num_t bclk, gpio_num_t ws, gpio_num_t dout, gpio_num_t din, gpio_num_t mclk=GPIO_NUM_NC)
    { 
        i2s_driver_ = new I2sDriver();
        i2s_driver_->setPins(bclk, ws, dout, din, mclk);
    }
    virtual ~AudioI2sDuplex();

    virtual bool Init(const audio_config_t &config) override;
    virtual uint32_t Write(const int16_t* data, uint32_t samples) override;
    virtual uint32_t Read(int16_t* dest, uint32_t samples) override;

protected:
    I2sDriver *i2s_driver_ = nullptr;
};

#endif // _AUDIO_I2S_DUPLEX_H

#endif //CONFIG_USE_AUDIO
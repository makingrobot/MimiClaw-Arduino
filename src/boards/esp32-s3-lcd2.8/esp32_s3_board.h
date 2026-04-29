#include "config.h"
#if BOARD_ESP32_S3_LCD_2_8 == 1

#ifndef _ESP32_S3_BOARD_H
#define _ESP32_S3_BOARD_H

#include <Arduino.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>

#include "src/framework/led/led.h"
#include "src/framework/file/file_system.h"
#include "src/framework/sys/frt_task.h"
#include "src/mimiclaw/mimi_board.h"

#if CONFIG_USE_DISPLAY==1
#include <Arduino_GFX_Library.h>
#include "src/framework/display/backlight.h"
#include "src/framework/display/disp_driver.h"
#endif

#if CONFIG_USE_AUDIO==1
#include "src/framework/audio/audio_codec.h"
#endif

class Esp32S3Board : public MimiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    Led *led_ = nullptr;
    FileSystem *filesystem_ = nullptr;

    void InitFileSystem();
    void InitButtons();
    void InitDisplay();
    void InitAudioCodec();

#if CONFIG_USE_DISPLAY==1
    Display* display_ = nullptr;
    Arduino_DataBus* gfx_bus_ = nullptr;
    Arduino_GFX* gfx_graphics_ = nullptr;
    Backlight* backlight_ = nullptr;
    DispDriver* disp_driver_ = nullptr;
#endif

#if CONFIG_USE_AUDIO==1
    AudioCodec *audio_codec_ = nullptr;
#endif

    FrtTask *btntick_task_;

public:
    Esp32S3Board();
    virtual Led* GetLed() override { return led_; }
    virtual FileSystem* GetFileSystem() override { return filesystem_; }

#if CONFIG_USE_DISPLAY==1
    virtual Display* GetDisplay() override { return display_; }
    virtual DispDriver* GetDispDriver() override { return disp_driver_; }
    virtual Backlight* GetBacklight() override { return backlight_; }
#endif

#if CONFIG_USE_AUDIO==1
    virtual AudioCodec* GetAudioCodec() override { return audio_codec_; }
#endif

};

#endif //_ESP32_S3_BOARD_H

#endif

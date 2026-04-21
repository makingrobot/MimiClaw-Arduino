#include "config.h"
#if BOARD_ESP32_S3_LCD_2_8 == 1

#ifndef _ESP32_S3_BOARD_H
#define _ESP32_S3_BOARD_H

#include <Arduino.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>

#include "src/libs/GFX_Library/Arduino_GFX_Library.h"

#include "src/framework/led/led.h"
#include "src/framework/file/file_system.h"
// #include "src/framework/audio/audio_codec.h"
#include "src/mimiclaw/mimi_board.h"

class Esp32S3Board : public MimiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    Led *led_ = nullptr;
    FileSystem *filesystem_ = nullptr;
    Display* display_ = nullptr;
    // AudioCodec *audio_codec_ = nullptr;

    Arduino_DataBus* gfx_bus_ = nullptr;
    Arduino_GFX* gfx_graphics_ = nullptr;

    void InitDisplay();
    void InitAudioCodec();
    void InitFileSystem();

public:
    Esp32S3Board();
    virtual Led* GetLed() override { return led_; }
    virtual FileSystem* GetFileSystem() override { return filesystem_; }
    virtual Display* GetDisplay() override { return display_; }
    // virtual AudioCodec* GetAudioCodec() override { return audio_codec_; }
    
};

#endif //_ESP32_S3_BOARD_H

#endif

#include "config.h"
#if BOARD_ESP32_S3_MOC == 1

#ifndef _ESP32_S3_BOARD_H
#define _ESP32_S3_BOARD_H

#include <Arduino.h>

#include "src/framework/led/led.h"
#include "src/framework/file/file_system.h"
#include "src/mimiclaw/mimi_board.h"

class Esp32S3Board : public MimiBoard {
private:
    Led *led_ = nullptr;
    FileSystem *filesystem_ = nullptr;

    void InitFileSystem();

public:
    Esp32S3Board();
    virtual Led* GetLed() override { return led_; }
    virtual FileSystem* GetFileSystem() override { return filesystem_; }

};

#endif //_ESP32_S3_BOARD_H

#endif

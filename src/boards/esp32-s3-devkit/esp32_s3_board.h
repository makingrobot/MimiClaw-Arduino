#include "config.h"
#if BOARD_ESP32_S3_DEVKIT == 1

#include "config.h"

#ifndef _ESP32_S3_BOARD_H
#define _ESP32_S3_BOARD_H

#include <Arduino.h>

#include "src/framework/board/wifi_board.h"
#include "src/framework/led/led.h"

class Esp32S3Board : public WifiBoard {
private:
    Led* led_ = nullptr;
    
public:
    Esp32S3Board();
    virtual Led* GetLed() override { return led_; }
};

#endif //_ESP32_S3_BOARD_H

#endif

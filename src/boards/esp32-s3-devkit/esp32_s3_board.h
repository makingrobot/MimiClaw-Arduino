#include "config.h"
#if BOARD_ESP32_S3_DEVKIT == 1

#include "config.h"

#ifndef _ESP32_S3_BOARD_H
#define _ESP32_S3_BOARD_H

#include <Arduino.h>

#include "src/framework/board/wifi_board.h"
#include "src/framework/led/led.h"
#include "src/framework/file/file_system.h"
#include "src/mimiclaw/mimi_tools.h"

class Esp32S3Board : public WifiBoard {
private:
    Led *led_ = nullptr;
    FileSystem *filesystem_ = nullptr;

    std::vector<const MimiTool*> _tools;

    void InitFileSystem();
    void InstallSkill();

public:
    Esp32S3Board();
    virtual Led* GetLed() override { return led_; }
    virtual FileSystem* GetFileSystem() override { return filesystem_; }

    std::vector<const MimiTool*>& tools() { return _tools; }

};

#endif //_ESP32_S3_BOARD_H

#endif

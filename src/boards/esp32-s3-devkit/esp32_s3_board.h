#include "config.h"
#if BOARD_ESP32_S3_DEVKIT == 1

#include "config.h"

#ifndef _ESP32_S3_BOARD_H
#define _ESP32_S3_BOARD_H

#include <vector>
#include <map>
#include <Arduino.h>

#include "src/framework/board/wifi_board.h"
#include "src/framework/led/led.h"
#include "src/framework/file/file_system.h"
#include "src/mimiclaw/mimi_tools.h"
#include "src/mimiclaw/mimi_skills.h"

class Esp32S3Board : public WifiBoard {
private:
    Led *led_ = nullptr;
    FileSystem *filesystem_ = nullptr;

    std::vector<const MimiTool*> _tools;
    std::vector<const SkillInfo*> _skills;

    void InitFileSystem();


public:
    Esp32S3Board();
    virtual Led* GetLed() override { return led_; }
    virtual FileSystem* GetFileSystem() override { return filesystem_; }

    std::vector<const MimiTool*>& tools() { return _tools; }
    std::vector<const SkillInfo*>& skills() { return _skills; }

    static std::vector<uint8_t> allow_pins;
};

#endif //_ESP32_S3_BOARD_H

#endif

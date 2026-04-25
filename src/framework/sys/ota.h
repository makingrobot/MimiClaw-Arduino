/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _OTA_H
#define _OTA_H

#include <Arduino.h>

class Ota {
public:
    bool CheckNewVersion();
    bool Upgrade();

private:
    String new_version_;
    String file_url_;
    uint32_t file_size_;
    String file_md5_;
};

#endif
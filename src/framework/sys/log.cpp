/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "log.h"

#include "config.h"

Log::Level Log::level = (Log::Level)CONFIG_LOG_LEVEL;

void Log::formatTime(char *buffer, uint8_t buf_size) {
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t seconds;
    uint16_t msec;

    uint32_t current = millis();
    uint32_t total_sec = current / 1000;
    msec = current - total_sec * 1000;

    // 天
    if (total_sec > 86400) {
        uint32_t all_day = total_sec / 86400;
        total_sec = total_sec - day * 86400;
        if (all_day > 100) {
            day = (uint8_t)(all_day % 100);
        }
    }

    // 小时
    if (total_sec > 3600) {
        hour = total_sec / 3600;
        total_sec = total_sec - hour * 3600;
    }

    // 分钟、秒
    if (total_sec > 60) {
        minute = total_sec / 60;
        seconds = total_sec - minute * 60;
    } else {
        seconds = total_sec;
    }

    snprintf(buffer, buf_size, "%02u:%02u:%02u:%02u.%03u", day, hour, minute, seconds, msec);
}
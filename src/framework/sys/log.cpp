/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "log.h"

#include "config.h"
#include <string>
#include <Arduino.h>

Log::Level Log::level = Log::WARN;

void Log::Info(const char* tag, const char* format, ...) {
    if (Log::level >= Log::INFO)
    {
        va_list args;
        va_start(args, format);
        char buffer[128] = { 0 };
        vsnprintf(buffer, 127, format, args);
        va_end(args);

#if CONFIG_USE_ESP_LOG==1
        log_i("[I]%s %s", tag, buffer);
#else
        Serial.printf("%d [I][%s] %s\n", millis(), tag, buffer);
#endif
    }
}

void Log::Warn(const char* tag, const char* format, ...) {
    if (Log::level >= Log::WARN)
    {
        va_list args;
        va_start(args, format);
        char buffer[128] = { 0 };
        vsnprintf(buffer, 127, format, args);
        va_end(args);

#if CONFIG_USE_ESP_LOG==1
        log_w("[W]%s %s", tag, buffer);
#else
        Serial.printf("%d [W][%s] %s\n", millis(), tag, buffer);
#endif
    }
}

void Log::Debug(const char* tag, const char* format, ...) {
    if (Log::level >= Log::DEBUG)
    {
        va_list args;
        va_start(args, format);
        char buffer[128] = { 0 };
        vsnprintf(buffer, 127, format, args);
        va_end(args);

#if CONFIG_USE_ESP_LOG==1
        log_d("[D]%s %s", tag, buffer);
#else
        Serial.printf("%d [D][%s] %s\n", millis(), tag, buffer);
#endif
    }
}

void Log::Error(const char* tag, const char* format, ...) {
    if (Log::level >= Log::ERROR)
    {
        va_list args;
        va_start(args, format);
        char buffer[128] = { 0 };
        vsnprintf(buffer, 127, format, args);
        va_end(args);

#if CONFIG_USE_ESP_LOG==1
        log_e("[E]%s %s", tag, buffer);
#else
        Serial.printf("%d [E][%s] %s\n", millis(), tag, buffer);
#endif
    }
}
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

const int LOG_BUFFER_LEN = 512;

void Log::Info(const char* tag, const char* format, ...) {
    if (Log::level >= Log::INFO)
    {
        va_list args;
        va_start(args, format);
        char buffer[LOG_BUFFER_LEN] = { 0 };
        vsnprintf(buffer, LOG_BUFFER_LEN-1, format, args);
        va_end(args);

#if CONFIG_USE_ESP_LOG==1
        log_i("%s %s", tag, buffer);
#else
        Serial.printf("I %d [%s] %s\n", millis(), tag, buffer);
#endif
    }
}

void Log::Warn(const char* tag, const char* format, ...) {
    if (Log::level >= Log::WARN)
    {
        va_list args;
        va_start(args, format);
        char buffer[LOG_BUFFER_LEN] = { 0 };
        vsnprintf(buffer, LOG_BUFFER_LEN-1, format, args);
        va_end(args);

#if CONFIG_USE_ESP_LOG==1
        log_w("%s %s", tag, buffer);
#else
        Serial.printf("W %d [%s] %s\n", millis(), tag, buffer);
#endif
    }
}

void Log::Debug(const char* tag, const char* format, ...) {
    if (Log::level >= Log::DEBUG)
    {
        va_list args;
        va_start(args, format);
        char buffer[LOG_BUFFER_LEN] = { 0 };
        vsnprintf(buffer, LOG_BUFFER_LEN-1, format, args);
        va_end(args);

#if CONFIG_USE_ESP_LOG==1
        log_d("%s %s", tag, buffer);
#else
        Serial.printf("D %d [%s] %s\n", millis(), tag, buffer);
#endif
    }
}

void Log::Error(const char* tag, const char* format, ...) {
    if (Log::level >= Log::ERROR)
    {
        va_list args;
        va_start(args, format);
        char buffer[LOG_BUFFER_LEN] = { 0 };
        vsnprintf(buffer, LOG_BUFFER_LEN-1, format, args);
        va_end(args);

#if CONFIG_USE_ESP_LOG==1
        log_e("%s %s", tag, buffer);
#else
        Serial.printf("E %d [%s] %s\n", millis(), tag, buffer);
#endif
    }
}
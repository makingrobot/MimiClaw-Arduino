/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _LOG_H
#define _LOG_H

#include <Arduino.h>

const int LOG_BUFFER_LEN = 256;

class Log {
public:

    enum Level { ERROR, INFO, WARN, DEBUG };

    static Level level;

    static void formatTime(char *buffer, uint8_t buf_size);

    template<typename... Args>
    static void Info(const char* tag, const char* format, Args... args){
        if (Log::level >= Log::INFO)
        {
            char timebuf[21] = {0};
            formatTime(timebuf, 20);
            char *buffer = (char *)malloc(LOG_BUFFER_LEN);
            if (!buffer) {
                Serial.printf("W[%s][log] Out of memory\n", timebuf);
                return;
            }
#if CONFIG_USE_ESP_LOG==1
            snprintf(buffer, LOG_BUFFER_LEN-1, "[%s] %s", tag, format);
            log_i(buffer, args...);
#else
            snprintf(buffer, LOG_BUFFER_LEN-1, "I[%s][%s] %s\n", timebuf, tag, format);
            Serial.printf(buffer, args...);
#endif
            free(buffer);
        }
    }

    template<typename... Args>
    static void Warn(const char* tag, const char* format, Args... args){
        if (Log::level >= Log::WARN)
        {
            char timebuf[21] = {0};
            formatTime(timebuf, 20);
            char *buffer = (char *)malloc(LOG_BUFFER_LEN);
            if (!buffer) {
                Serial.printf("W[%s][log] Out of memory\n",  timebuf);
                return;
            }
#if CONFIG_USE_ESP_LOG==1
            snprintf(buffer, LOG_BUFFER_LEN-1, "[%s] %s", tag, format);
            log_w(buffer, args...);
#else
            snprintf(buffer, LOG_BUFFER_LEN-1, "W[%s][%s] %s\n", timebuf, tag, format);
            Serial.printf(buffer, args...);
#endif
            free(buffer);
        }
    }

    template<typename... Args>
    static void Debug(const char* tag, const char* format, Args... args){
        if (Log::level >= Log::DEBUG)
        {
            char timebuf[21] = {0};
            formatTime(timebuf, 20);
            char *buffer = (char *)malloc(LOG_BUFFER_LEN);
            if (!buffer) {
                Serial.printf("W[%s][log] Out of memory\n", timebuf);
                return;
            }
#if CONFIG_USE_ESP_LOG==1
            snprintf(buffer, LOG_BUFFER_LEN-1, "[%s][%s][%d] %s", timebuf, tag, xPortGetCoreID(), format);
            log_d(buffer, args...);
#else
            snprintf(buffer, LOG_BUFFER_LEN-1, "D[%s][%s][%d] %s\n", timebuf, tag, xPortGetCoreID(), format);
            Serial.printf(buffer, args...);
#endif
            free(buffer);
        }
    }

    template<typename... Args>
    static void Debug(const char* tag, uint32_t line, const char* format, Args... args){
        if (Log::level >= Log::DEBUG)
        {
            char timebuf[21] = {0};
            formatTime(timebuf, 20);
            char *buffer = (char *)malloc(LOG_BUFFER_LEN);
            if (!buffer) {
                Serial.printf("W[%s][log] Out of memory\n", timebuf);
                return;
            }
#if CONFIG_USE_ESP_LOG==1
            snprintf(buffer, LOG_BUFFER_LEN-1, "[%s][%s_%u][%d] %s", timebuf, tag, line, xPortGetCoreID(), format);
            log_d(buffer, args...);
#else
            snprintf(buffer, LOG_BUFFER_LEN-1, "D[%s][%s_%u][%d] %s\n", timebuf, tag, line, xPortGetCoreID(), format);
            Serial.printf(buffer, args...);
#endif
            free(buffer);
        }
    }

    template<typename... Args>
    static void Error(const char* tag, const char* format, Args... args){
        if (Log::level >= Log::ERROR)
        {
            char timebuf[21] = {0};
            formatTime(timebuf, 20);
            char *buffer = (char *)malloc(LOG_BUFFER_LEN);
            if (!buffer) {
                Serial.printf("W[%s][log] Out of memory\n", timebuf);
                return;
            }
#if CONFIG_USE_ESP_LOG==1
            snprintf(buffer, LOG_BUFFER_LEN-1, "[%s][%s][%d] %s", timebuf, tag, xPortGetCoreID(), format);
            log_e(buffer, args...);
#else
            snprintf(buffer, LOG_BUFFER_LEN-1, "E[%s][%s][%d] %s\n", timebuf, tag, xPortGetCoreID(), format);
            Serial.printf(buffer, args...);
#endif
            free(buffer);
        }
    }
    
    template<typename... Args>
    static void Error(const char* tag, uint32_t line, const char* format, Args... args){
        if (Log::level >= Log::ERROR)
        {
            char timebuf[21] = {0};
            formatTime(timebuf, 20);
            char *buffer = (char *)malloc(LOG_BUFFER_LEN);
            if (!buffer) {
                Serial.printf("W[%s][log] Out of memory\n", timebuf);
                return;
            }
#if CONFIG_USE_ESP_LOG==1
            snprintf(buffer, LOG_BUFFER_LEN-1, "[%s][%s_%u][%d] %s", timebuf, tag, line, xPortGetCoreID(), format);
            log_e(buffer, args...);
#else
            snprintf(buffer, LOG_BUFFER_LEN-1, "E[%s][%s_%u][%d] %s\n", timebuf, tag, line, xPortGetCoreID(), format);
            Serial.printf(buffer, args...);
#endif
            free(buffer);
        }
    }
};

#endif //_LOG_H
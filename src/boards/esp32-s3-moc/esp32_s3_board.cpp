#include "config.h"
#if BOARD_ESP32_S3_MOC == 1

#include "driver/gpio.h"
#include "board_config.h"
#include "esp32_s3_board.h"
#include "src/framework/led/ws2812_led.h"
#include <Arduino.h>

#if CONFIS_USE_SPIFFS==1
#include <SPIFFS.h>
#elif CONFIG_USE_SDFS==1
#include <SPI.h>
#include <SD.h>
#endif

#define TAG "Esp32S3Board"

/**
 * 19,20引脚用作USB，未引出
 */
std::vector<uint8_t> MimiBoard::allow_pins = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 
        10, 11, 12, 13, 14, 15, 16, 17, 18,
        21, 
        35, 36, 37, 38, 39, 
        40, 41, 42, 45, 46, 47, 48
    };

void* create_board() { 
    return new Esp32S3Board();
}

Esp32S3Board::Esp32S3Board() : MimiBoard() {
    Log::Info(TAG, "===== Create Board ...... =====");

    Log::Info(TAG, "initial led.");
    led_ = new Ws2812Led(BUILTIN_LED_PIN, 1);

    InitFileSystem();
    Log::Info( TAG, "===== Board config completed. =====");
}

void Esp32S3Board::InitFileSystem() {
    Log::Info( TAG, "Init file system ......" );

#if CONFIS_USE_SPIFFS==1
    if (!SPIFFS.begin(true)) {
        Log::Error(TAG, __LINE__, "SPIFFS Mount Failed");
        if(!SPIFFS.format()){
            Log::Error(TAG, "SPIFFS format failed");
            return;
        }
        SPIFFS.begin();
    }

    filesystem_ = new FileSystem(SPIFFS);
    filesystem_->SetTotalBytes(SPIFFS.totalBytes());
    filesystem_->SetFreeBytes(SPIFFS.totalBytes() - SPIFFS.usedBytes());
    filesystem_->SetType("SPIFFS");

#elif CONFIG_USE_SDFS==1
    SPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN); // config in board_config.h
    if (!SD.begin(SD_CS_PIN)) {
        Log::Info(TAG, "SD Mount Failed");
        return;
    }

    filesystem_ = new FileSystem(SD);
    filesystem_->setTotalBytes(SD.totalBytes());
    filesystem_->setFreeBytes(SD.totalBytes() - SD.usedBytes());
    filesystem_->setType("SDFS");
#endif
}

#endif
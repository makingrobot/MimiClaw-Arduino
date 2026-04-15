#include "config.h"
#if BOARD_ESP32_S3_DEVKIT == 1

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

void* create_board() { 
    return new Esp32S3Board();
}

Esp32S3Board::Esp32S3Board() : WifiBoard() {

    Log::Info(TAG, "===== Create Board ...... =====");

    Log::Info(TAG, "initial led.");
    led_ = new Ws2812Led(BUILTIN_LED_PIN, 1);

    InitFileSystem();
    Log::Info( TAG, "===== Board config completed. =====");
}

void Esp32S3Board::InitFileSystem() {
    Log::Info( TAG, "Init file system ......" );

#if CONFIS_USE_SPIFFS==1
    if (!SPIFFS.begin()) {
        Log::Error(TAG, "SPIFFS Mount Failed");
        return;
    }

    filesystem_ = new FileSystem(SPIFFS);
    filesystem_->setTotalBytes(SPIFFS.totalBytes());
    filesystem_->setFreeBytes(SPIFFS.totalBytes() - SPIFFS.usedBytes());
    filesystem_->setType("SPIFFS");

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
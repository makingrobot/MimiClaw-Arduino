#include "config.h"
#if BOARD_ESP32_S3_DEVKIT == 1

#include "board_config.h"
#include "esp32_s3_board.h"
#include "src/framework/led/ws2812_led.h"

#define TAG "Esp32S3Board"

void* create_board() { 
    return new Esp32S3Board();
}

Esp32S3Board::Esp32S3Board() : WifiBoard() {

    Log::Info(TAG, "===== Create Board ...... =====");

    Log::Info(TAG, "initial led.");
    led_ = new Ws2812Led(BUILTIN_LED_PIN, 1);

    Log::Info( TAG, "===== Board config completed. =====");
}

#endif
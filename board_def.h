/**
 * MimiClaw Arduino
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _BOARD_DEF_H
#define _BOARD_DEF_H

#include "config.h"

#if BOARD_ESP32_S3_DEVKIT == 1 
#define BOARD_NAME "esp32-s3-devkit"
#include "src/boards/esp32-s3-devkit/esp32_s3_board.h"

#endif


#endif
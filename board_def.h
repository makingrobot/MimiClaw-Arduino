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

#elif BOARD_ESP32_S3_MOC == 1 
#define BOARD_NAME "esp32-s3-moc"
#include "src/boards/esp32-s3-moc/esp32_s3_board.h"

#elif BOARD_ESP32_S3_LCD_2_8 == 1 
#define BOARD_NAME "esp32-s3-lcd2.8"
#include "src/boards/esp32-s3-lcd2.8/esp32_s3_board.h"

#endif


#endif
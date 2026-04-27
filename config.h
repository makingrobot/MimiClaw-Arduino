/**
 * MimiClaw Arduino
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _CONFIG_H
#define _CONFIG_H

// 板子配置
#define BOARD_ESP32_S3_DEVKIT        1

#if BOARD_ESP32_S3_DEVKIT == 1
#include "src/boards/esp32-s3-devkit/board_config.h"
#include "src/boards/esp32-s3-devkit/app_config_ext.h"

#elif BOARD_ESP32_S3_MOC == 1
#include "src/boards/esp32-s3-moc/board_config.h"
#include "src/boards/esp32-s3-moc/app_config_ext.h"

#elif BOARD_ESP32_S3_LCD_2_8 == 1
#include "src/boards/esp32-s3-lcd2.8/board_config.h"
#include "src/boards/esp32-s3-lcd2.8/app_config_ext.h"

#endif

// 应用配置
#include "src/mimiclaw/app_config.h"

#endif //_CONFIG_H
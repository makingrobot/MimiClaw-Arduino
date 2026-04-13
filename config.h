/**
 * MimiClaw Arduino
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _CONFIG_H
#define _CONFIG_H

// 板子型号
#define BOARD_ESP32_S3_DEVKIT        1

#include "src/mimiclaw/app_config.h"

#if BOARD_ESP32_S3_DEVKIT == 1
#include "src/boards/esp32-s3-devkit/board_config.h"

#endif

#endif //_CONFIG_H
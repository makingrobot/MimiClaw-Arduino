#if BOARD_ESP32_S3_DEVKIT == 1

#ifndef _BOARD_CONFIG_H
#define _BOARD_CONFIG_H

#include <driver/gpio.h>

/*****
 * 本文件主要用于配置硬件相关的设置，如引脚
 */

#define LED_PIN                             GPIO_NUM_9

#define SD_CLK_PIN                          GPIO_NUM_NC 
#define SD_MISO_PIN                         GPIO_NUM_NC 
#define SD_MOSI_PIN                         GPIO_NUM_NC
#define SD_CS_PIN                           GPIO_NUM_NC

// LED驱动
#define CONFIG_USE_LED_GPIO                 1
#define CONFIG_USE_LED_WS2812               1


#endif //_BOARD_CONFIG_H

#endif

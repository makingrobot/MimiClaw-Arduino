#if BOARD_ESP32_S3_LCD_2_8 == 1

#ifndef _BOARD_CONFIG_H
#define _BOARD_CONFIG_H

#include <driver/gpio.h>

/*****
 * 本文件主要用于配置硬件相关的设置，如引脚
 */

#define BUILTIN_LED_PIN                     GPIO_NUM_42

// I2c
#define I2C_SDA_PIN                         GPIO_NUM_16
#define I2C_SCL_PIN                         GPIO_NUM_15

// TFT-LCD（ILI9341 或 ST7789）
#define DISPLAY_CS_PIN                      GPIO_NUM_10
#define DISPLAY_DC_PIN                      GPIO_NUM_46
#define DISPLAY_CLK_PIN                     GPIO_NUM_12
#define DISPLAY_MOSI_PIN                    GPIO_NUM_11
#define DISPLAY_MISO_PIN                    GPIO_NUM_13
#define DISPLAY_RST_PIN                     GPIO_NUM_NC
#define DISPLAY_LED_PIN                     GPIO_NUM_46

#define DISPLAY_WIDTH                       240
#define DISPLAY_HEIGHT                      320
#define DISPLAY_MIRROR_X                    true
#define DISPLAY_MIRROR_Y                    false
#define DISPLAY_SWAP_XY                     false
#define DISPLAY_INVERT_COLOR                true
#define DISPLAY_RGB_ORDER                   LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_OFFSET_X                    0
#define DISPLAY_OFFSET_Y                    0
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT     false
#define DISPLAY_SPI_MODE                    0

// Touch
#define TOUCH_SCL_PIN                       GPIO_NUM_16
#define TOUCH_SDA_PIN                       GPIO_NUM_15
#define TOUCH_RST_PIN                       GPIO_NUM_18
#define TOUCH_INT_PIN                       GPIO_NUM_17

// 音频
#define AUDIO_I2S_MCLK_PIN                  GPIO_NUM_4
#define AUDIO_I2S_BCLK_PIN                  GPIO_NUM_5
#define AUDIO_I2S_LRC_PIN                   GPIO_NUM_7
#define AUDIO_I2S_DOUT_PIN                  GPIO_NUM_6
#define AUDIO_I2S_DIN_PIN                   GPIO_NUM_8
#define AUDIO_CODEC_PA_PIN                  GPIO_NUM_1
#define AUDIO_CODEC_ES8311_ADDR             0x00

#define AUDIO_INPUT_SAMPLE_RATE             16000
#define AUDIO_OUTPUT_SAMPLE_RATE            16000

// SD卡
#define SD_MMC_CLK_PIN                      GPIO_NUM_38 
#define SD_MMC_CMD_PIN                      GPIO_NUM_40 
#define SD_MMC_D1_PIN                       GPIO_NUM_39
#define SD_MMC_D2_PIN                       GPIO_NUM_41
#define SD_MMC_D3_PIN                       GPIO_NUM_48
#define SD_MMC_D4_PIN                       GPIO_NUM_47

// 电池电压ADC输入
#define BATTERY_ADC_PIN                     GPIO_NUM_9

#define CONFIG_AUDIO_CODEC_ES8311           1

// LED驱动
#define CONFIG_USE_LED_GPIO                 1
#define CONFIG_USE_LED_WS2812               1

#endif //_BOARD_CONFIG_H

#endif

#ifndef APP_CONFIG_EXT_H
#define APP_CONFIG_EXT_H


#define PRODUCT_ID             "esp32-s3-lcd2.8"

// UART1引脚
#define CONFIG_UART1_RX                     43
#define CONFIG_UART1_TX                     44

// 存储
//#define CONFIS_USE_SPIFFS                   0
//#define CONFIG_USE_SDFS                     1

// 显示
#define CONFIG_USE_DISPLAY                  1 
#define CONFIG_USE_GFX_LIBRARY              1
#define CONFIG_USE_LVGL                     1

// 音频
#define CONFIG_USE_AUDIO                    0

#endif
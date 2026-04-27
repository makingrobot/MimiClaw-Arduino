#ifndef APP_CONFIG_EXT_H
#define APP_CONFIG_EXT_H

#define PRODUCT_MODEL                       "mimiclaw-lcd2.8"
#define PRODUCT_TOKEN                       "f9627c44e1b7ad2ba7579a920ced3a7a"

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
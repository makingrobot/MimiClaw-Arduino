/**
 * MimiClaw-Arduino
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _APP_CONFIG_H
#define _APP_CONFIG_H

/*****
 * 本文件主要用于启用某项特性，或使用某个硬件。
 * 对开发框架源码裁剪的配置项均在本文件中，不使用的硬件和特性请不要启用，以得到最小可用的代码。
 * 注意：任一选项的更改都可能会导致项目源文件的整体编译！
 *
 * 此配置文件内的选项会影响所有开发板
 * 若只对某个开发板要定制选项，请在开发板文件夹内使用 app_config_ext.h 来覆盖选项。
 */

#define APP_VERSION                          "1.0.0"

#define YUNLC_API_BASE                       "http://api.yunlc.com.cn"

#ifndef PRODUCT_MODEL
#define PRODUCT_MODEL                       "" //产品型号
#endif
#ifndef PRODUCT_TOKEN
#define PRODUCT_TOKEN                       "" //产品Token
#endif

// 定时器，默认使用FreeRTOS定时器
#ifndef CONFIG_USE_SW_TIMER
#define CONFIG_USE_SW_TIMER                 1
#endif
#ifndef CONFIG_USE_HW_TIMER
#define CONFIG_USE_HW_TIMER                 0  // esp_timer
#endif

// UART，控制台默认使用UART1
#define CONFIG_USE_UART1                    1
#ifndef CONFIG_UART1_RX
#define CONFIG_UART1_RX                     18
#endif
#ifndef CONFIG_UART1_TX
#define CONFIG_UART1_TX                     17
#endif

// 不使用开发框架提供的WiFi功能
#define CONFIG_USE_WIFI                     0

// 文件系统，默认使用SPIFFS
#define CONFIG_USE_FS                       1
#ifndef CONFIS_USE_SPIFFS
#define CONFIS_USE_SPIFFS                   1
#endif
#ifndef CONFIG_USE_SDFS
#define CONFIG_USE_SDFS                     0
#endif

// 音频，默认不使用
#ifndef CONFIG_USE_AUDIO
#define CONFIG_USE_AUDIO                    0
#endif

// 显示，默认不使用
#ifndef CONFIG_USE_DISPLAY
#define CONFIG_USE_DISPLAY                  0 
#endif

// 使用ESP_LOG
#ifndef CONFIG_LOG_LEVEL
#define CONFIG_LOG_LEVEL                    3  //1:info, 2:warn, 3:debug
#endif
#ifndef CONFIG_USE_ESP_LOG
#define CONFIG_USE_ESP_LOG                  0
#endif

#endif //_APP_CONFIG_H

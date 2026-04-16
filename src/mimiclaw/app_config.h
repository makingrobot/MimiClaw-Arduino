/**
 * MimiClaw Arduino
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _APP_CONFIG_H
#define _APP_CONFIG_H

/*****
 * 本文件主要用于启用某项特性，或使用某个硬件。
 * 对开发框架源码裁剪的配置项均在本文件中，不使用的硬件和特性请不要启用，以得到最小可用的代码。
 * 注意：任一选项的更改都可能会导致项目源文件的整体编译！
 */

 // 定时器
#define CONFIG_USE_SW_TIMER                 1
#define CONFIG_USE_HW_TIMER                 0  // esp_timer

// 使用WiFi
#define CONFIG_USE_WIFI                     1

// 使用文件系统
#define CONFIG_USE_FS                       1
#define CONFIS_USE_SPIFFS                   1
#define CONFIG_USE_SDFS                     0  // 使用SDCard

// 使用音频
#define CONFIG_USE_AUDIO                    0

// 使用显示
#define CONFIG_USE_DISPLAY                  0   // 显示
#define CONFIG_USE_GFX_LIBRARY              0   // 使用GFX_Library图形库

// LVGL
#define CONFIG_USE_LVGL                     0  // LVGL
#define LV_LVGL_H_INCLUDE_SIMPLE            0

// 使用ESP_LOG
#define CONFIG_LOG_LEVEL                    3  //1:info, 2:warn, 3:debug
#define CONFIG_USE_ESP_LOG                  0

#endif //_APP_CONFIG_H

/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_GFX_LIBRARY == 1 && CONFIG_USE_LVGL == 1

#ifndef _GFX_LVGL_DRIVER_H
#define _GFX_LVGL_DRIVER_H

#include "disp_driver.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include "../sys/frt_task.h"

class GfxLvglDriver : public DispDriver {
public:
    GfxLvglDriver(Arduino_GFX* gfx, int width, int height, Touch* touch=nullptr);

    virtual ~GfxLvglDriver() { 
        if (display_ != nullptr) {
            lv_display_delete(display_);
        }
    }

    void Init() override;
    void Flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
    void TouchRead(lv_indev_t *indev, lv_indev_data_t *data);
    void Loop();

private:
    Arduino_GFX* gfx_;

    lv_display_t *display_ = nullptr;
    lv_color_t *disp_buf_ = nullptr;
    lv_indev_t *indev_ = nullptr;

    FrtTask *lvgl_task_ = nullptr;
};

#endif //_GFX_LVGL_DRIVER_H

#endif //CONFIG_USE_GFX_LIBRARY
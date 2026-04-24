/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if BOARD_ESP32_S3_LCD_2_8==1 && CONFIG_USE_LVGL == 1

#ifndef _FT6336_TOUCH_H
#define _FT6336_TOUCH_H

#include "src/framework/display/touch.h"
#include <FT6336.h>

#define TOUCH_FT6336
#define TOUCH_MAP_X1 0
#define TOUCH_MAP_X2 320
#define TOUCH_MAP_Y1 0
#define TOUCH_MAP_Y2 240

class Ft6336Touch: public Touch {
public: 
    Ft6336Touch(gpio_num_t sda_pin, gpio_num_t scl_pin, gpio_num_t int_pin, gpio_num_t rst_pin) 
    {
        ts_ = new FT6336(sda_pin, scl_pin, int_pin, rst_pin, 
            max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));
    }

    virtual void Init(uint16_t w, uint16_t h, uint8_t r) override
    {
        width_ = w; 
        height_ = h;
        switch (r){
            case ROTATION_NORMAL:
            case ROTATION_INVERTED:
                min_x_ = TOUCH_MAP_X1;
                max_x_ = TOUCH_MAP_X2;
                min_y_ = TOUCH_MAP_Y1;
                max_y_ = TOUCH_MAP_Y2;
                break;
            case ROTATION_LEFT:
            case ROTATION_RIGHT:
                min_x_ = TOUCH_MAP_Y1;
                max_x_ = TOUCH_MAP_Y2;
                min_y_ = TOUCH_MAP_X1;
                max_y_ = TOUCH_MAP_X2;
                break;
            default:
                break;
        }
        ts_->begin();
        ts_->setRotation(r);
    }

    virtual bool IsTouched(void) override
    {
        ts_->read();
        if (ts_->isTouched)
        {
        //#if defined(TOUCH_SWAP_XY)
        //    touch_last_x = map(ts.points[0].y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, width - 1);
        //    touch_last_y = map(ts.points[0].x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, height - 1);
        //#else
            last_x_ = map(ts_->points[0].x, min_x_, max_x_, 0, width_ - 1);
            last_y_ = map(ts_->points[0].y, min_y_, max_y_, 0, height_ - 1);
        //#endif
            return true;
        }
        else
        {
            return false;
        }
    }

    virtual bool HasSignal(void) override
    {
        return true;
    }

    virtual bool IsReleased(void) override
    {
        return true;
    }

private:
    FT6336 *ts_ = nullptr;

    uint16_t width_=0, height_=0; 
    uint8_t rotation_;
    uint16_t min_x_=0, max_x_=0;
    uint16_t min_y_=0, max_y_=0;

};

#endif

#endif
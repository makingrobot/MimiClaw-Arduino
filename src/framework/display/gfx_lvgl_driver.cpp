/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_GFX_LIBRARY == 1 && CONFIG_USE_LVGL == 1

#include "gfx_lvgl_driver.h"
#include "../sys/log.h"

#define TAG "GfxLvglDriver"

static GfxLvglDriver* g_driver;

#if LV_USE_LOG != 0
void _log_print_cb(lv_log_level_t level, const char *buf)
{
    LV_UNUSED(level);
    Log::INFO(TAG, buf);
}
#endif

GfxLvglDriver::GfxLvglDriver(Arduino_GFX* gfx, int width, int height, Touch* touch)
        : DispDriver(width, height), gfx_(gfx) { 
    g_driver = this;
    touch_ = touch;
}

uint32_t _millis_cb(void)
{
    return millis();
}

/* LVGL calls it when a rendered image needs to copied to the display*/
void _flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    g_driver->Flush(disp, area, px_map);
}

/*Read the touchpad*/
void _touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    g_driver->TouchRead(indev, data);
}

void GfxLvglDriver::Init() {
    Log::Info(TAG, "gfx lvgl driver init.");
    
    gfx_->fillScreen(BLACK);

    if (touch_) {
        touch_->Init(width_, height_, 1);
    }

    lv_init();

    /*Set a tick source so that LVGL will know how much time elapsed. */
    lv_tick_set_cb(_millis_cb);

  /* register print function for debugging */
#if LV_USE_LOG != 0
    lv_log_register_print_cb(_log_print_cb);
#endif

    int screen_width = gfx_->width();
    int screen_height = gfx_->height();
    int buf_size = 0;

#ifdef DIRECT_RENDER_MODE
    buf_size = screen_width * screen_height;
#else
    buf_size = screen_width * 40;
#endif

#if defined(DIRECT_MODE) && (defined(CANVAS) || defined(RGB_PANEL))
    disp_buf_ = (lv_color_t *)gfx->getFramebuffer();
#else  // !(defined(DIRECT_MODE) && (defined(CANVAS) || defined(RGB_PANEL)))
    disp_buf_ = (lv_color_t *)heap_caps_malloc(buf_size * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_buf_)
    {
        // remove MALLOC_CAP_INTERNAL flag try again
        disp_buf_ = (lv_color_t *)heap_caps_malloc(buf_size * 2, MALLOC_CAP_8BIT);
    }
#endif // !(defined(DIRECT_MODE) && (defined(CANVAS) || defined(RGB_PANEL)))

    if (!disp_buf_) {
        Log::Error(TAG, "LVGL disp_buf allocate failed!");
        return;
    }
   
    display_ = lv_display_create(screen_width, screen_height);
    lv_display_set_flush_cb(display_, _flush_cb);

#ifdef DIRECT_MODE
    lv_display_set_buffers(display_, disp_buf_, NULL, buf_size * 2, LV_DISPLAY_RENDER_MODE_DIRECT);
#else
    lv_display_set_buffers(display_, disp_buf_, NULL, buf_size * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

    if (touch_) {
        /*Initialize the (dummy) input device driver*/
        indev_ = lv_indev_create();
        lv_indev_set_type(indev_, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
        lv_indev_set_read_cb(indev_, _touchpad_read);
    }

    lvgl_task_ = new FrtTask("lvgl_task");
    lvgl_task_->OnLoop([this](){
        Loop();
        vTaskDelay(pdMS_TO_TICKS(5));
    });
    lvgl_task_->Start(8192, 3);
}

void GfxLvglDriver::Flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
#ifndef DIRECT_MODE
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    gfx_->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
#endif // #ifndef DIRECT_MODE

    /*Call it to tell LVGL you are ready*/
    lv_disp_flush_ready(disp);
}

void GfxLvglDriver::TouchRead(lv_indev_t *indev, lv_indev_data_t *data) {
    if (touch_->HasSignal())
    {
        if (touch_->IsTouched())
        {
            data->state = LV_INDEV_STATE_PRESSED;

            /*Set the coordinates*/
            data->point.x = touch_->last_x();
            data->point.y = touch_->last_y();
        }
        else if (touch_->IsReleased())
        {
            data->state = LV_INDEV_STATE_RELEASED;
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void GfxLvglDriver::Loop() {
    lv_task_handler();

#ifdef DIRECT_MODE
#if defined(CANVAS) || defined(RGB_PANEL)
    gfx_->flush();
#else // !(defined(CANVAS) || defined(RGB_PANEL))
    gfx_->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#endif // !(defined(CANVAS) || defined(RGB_PANEL))
#else  // !DIRECT_MODE
#ifdef CANVAS
    gfx_->flush();
#endif
#endif // !DIRECT_MODE
}

#endif //CONFIG_USE_GFX_LIBRARY
/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _TOUCH_H
#define _TOUCH_H

class Touch {
public:
    virtual void Init(uint16_t width, uint16_t height, uint8_t rotation) = 0;
    virtual bool HasSignal() = 0;
    virtual bool IsTouched() = 0;
    virtual bool IsReleased() = 0;

    inline int last_x() const { return last_x_; }
    inline int last_y() const { return last_y_; }

protected:
    int last_x_;
    int last_y_;

};

#endif
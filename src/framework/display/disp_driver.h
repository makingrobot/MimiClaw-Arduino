/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"

#ifndef _DISP_DRIVER_H
#define _DISP_DRIVER_H

enum DriveMode {
    kDriveModeUnknown,
    kDriveModeI2c,
    kDriveModeSpi,
    kDriveModeRgb,
    kDriveModeMipi,
};

class DispDriver {
public:
    DispDriver(int width, int height) 
        : width_(width), height_(height) { }
    virtual ~DispDriver() { }

    inline int width() const { return width_; }
    inline int height() const { return height_; }

    DriveMode GetMode() const { return drive_mode_; }
    virtual void Init() = 0;

protected:

    DriveMode drive_mode_;

    const int width_;
    const int height_;

};

#endif //_DISP_DRIVER_H

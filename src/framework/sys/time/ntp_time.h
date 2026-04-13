#include "config.h"
#if CONFIG_USE_WIFI==1

#ifndef _NTP_TIME_H
#define _NTP_TIME_H

#include "../time.h"
#include <cstdint>
#include <WiFi.h>

class NTPTime : public Time {
public:
    NTPTime();
    ~NTPTime();

    std::string GetHourMinute() override;
    void Update() override;

private:
    struct tm time_info_;

};

#endif //_TIME_H

#endif
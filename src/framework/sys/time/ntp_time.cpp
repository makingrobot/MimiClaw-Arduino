#include "config.h"
#if CONFIG_USE_WIFI==1

#include "ntp_time.h"

#include "../../sys/log.h"

#define TAG "NTPTimer"

const char* ntpServer = "ntp1.aliyun.com";
const uint32_t GmtOffsetSeconds = 8 * 3600;
const uint32_t DayLightOffsetSeconds = 0;

NTPTime::NTPTime() {
    configTime(GmtOffsetSeconds, DayLightOffsetSeconds, ntpServer);
}

NTPTime::~NTPTime() {
}

std::string NTPTime::GetHourMinute() {
    char buf[10];
    sprintf(buf, "%02d:%02d", time_info_.tm_hour, time_info_.tm_sec);
    return std::string(buf);
}

void NTPTime::Update() {
    if (!getLocalTime(&time_info_, 50)) { // try 5 times.
        Log::Info(TAG, "Failed to obtain time.");
    }
}

#endif
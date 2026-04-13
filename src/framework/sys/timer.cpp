#include "timer.h"

#include "config.h"
#include "timer/frt_timer.h"
#include "timer/hw_timer.h"
#include "log.h"

#define TAG "Timer"

Timer* TimerFactory::CreateTimer(const std::string& name) {
#if CONFIG_USE_SW_TIMER==1
    return new FrtTimer(name);
#elif CONFIG_USE_HW_TIMER==1
    return new HwTimer(name);
#else
    Log::Error(TAG, "please define CONFIG_USE_SW_TIMER or CONFIG_USE_HW_TIMER.");
    return nullptr;
#endif
}
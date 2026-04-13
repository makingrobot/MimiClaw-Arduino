#include "config.h"
#if CONFIG_USE_SW_TIMER==1

#ifndef _FRT_TIMER_H
#define _FRT_TIMER_H

#include <functional>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "../timer.h"

class FrtTimer : public Timer {
public:
    FrtTimer(const std::string& name);
    virtual ~FrtTimer();

    bool Start(uint32_t interval_ms, std::function<void()> callback_function, bool once=false) override;
    bool Stop() override;

    std::function<void()> callback_function_;

private:
    TimerHandle_t timer_handle_ = NULL;
    std::string name_;
    
};

#endif //_FRT_TIMER_H

#endif
#include "config.h"
#if CONFIG_USE_HW_TIMER==1

#ifndef _HW_TIMER_H
#define _HW_TIMER_H

#include <functional>
#include <string>
#include <Ticker.h>
#include "../timer.h"

class HwTimer : public Timer {
public:
    HwTimer(const std::string& name);
    virtual ~HwTimer();

    bool Start(uint32_t interval_ms, std::function<void()> callback_function, bool once=false) override;
    bool Stop() override;

    std::function<void()> callback_function_;

private:
    Ticker* ticker_ = nullptr;
    bool attached_ = false;
    std::string name_;
    
};

#endif //_HW_TIMER_H

#endif
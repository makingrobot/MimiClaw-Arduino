#include "config.h"
#if CONFIG_USE_HW_TIMER==1

#include "hw_timer.h"
#include "../log.h"

#define TAG "HwTimer"

HwTimer::HwTimer(const std::string& name) : name_(name) {
    ticker_ = new Ticker();
}

HwTimer::~HwTimer() {
    Stop();

    delete ticker_;
}

bool HwTimer::Start(uint32_t interval_ms, std::function<void()> callback_function, bool once) {
    Stop();

    callback_function_ = std::move(callback_function);

    if (once) {
        ticker_->once_ms(interval_ms, callback_function_);
    } else {
        ticker_->attach_ms(interval_ms, callback_function_);
    } 
  
    return true;
}

bool HwTimer::Stop() {
    if (attached_) {
        ticker_->detach();
        return true;
    }
    return false;
}

#endif
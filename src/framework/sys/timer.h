
#ifndef _TIMER_H
#define _TIMER_H

#include <functional>
#include <cstdint>
#include <string>

class Timer {
public:

    /**
     * 启动定时器
     * interval_ms 触发间隔时间（毫秒）
     * callback_function 解发回调函数
     * once 是否只执行一次，默认为false
     */
    virtual bool Start(uint32_t interval_ms, std::function<void()> callback_function, bool once=false) = 0;
    virtual bool Stop() = 0;


};

class TimerFactory {
public:
    static Timer* CreateTimer(const std::string& name);
};

#endif //_TIMER_H
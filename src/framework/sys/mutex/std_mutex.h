/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _STD_MUTEX_H
#define _STD_MUTEX_H

#include <mutex>
#include "../mutex.h"

class StdMutex :  public Mutex {
public:
    StdMutex() 
    {
    }

    bool Lock(int timeout_ms = 0) override 
    {
        mutex_.lock();
        return true;
    }

    void Unlock() override 
    {
        mutex_.unlock();
    }

private:
    std::mutex mutex_;

};

#endif  //_STD_MUTEX_H
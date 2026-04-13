/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _MUTEX_H
#define _MUTEX_H

#include "log.h"

class Mutex {
public:
    virtual bool Lock(int timeout_ms) = 0;
    virtual void Unlock() = 0;
};

class MutexGuard {
public:
    MutexGuard(Mutex *mutex, int timeout_ms=0) : mutex_(mutex) {
        is_locked_ = mutex_->Lock(timeout_ms);
        if (!is_locked_) 
        {
            Log::Error("Mutex", "Failed to lock mutex");
        }
    }
    
    bool UnLock() 
    {
        if (is_locked_) 
        {
            mutex_->Unlock();
            is_locked_ = false;
            return true;
        }
        return false;
    }

    ~MutexGuard() {
        if (is_locked_) 
        {
            mutex_->Unlock();
        }
    }

    MutexGuard(const MutexGuard&) = delete;
    MutexGuard& operator=(const MutexGuard&) = delete;

    const bool IsLocked() const { return is_locked_; }
private:
    Mutex *mutex_;
    bool is_locked_;
};

#endif  //_MUTEX_H
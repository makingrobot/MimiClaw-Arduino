/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_FILTER_H
#define _AUDIO_FILTER_H

#include "audio_common.h"

/**
 * 音频过滤基类
 * 
 * 用于音频数据处理、音频数据混合、音频数据特效等需要数据转换的场景
 * 即实现 音频数据A -> 音频数据B 的变换目标
 * 
 * 待实现
 */
class AudioFilter {
public:
    virtual bool DoFilter(const filter_data_t &in_data, filter_data_t &out_data) = 0;
    
    inline int order() const { return order_; }
    void SetOrder(int order) { order_ = order; }

    bool operator<(const AudioFilter& other) const 
    {
        return order_ < other.order();
    }

private:
    int order_;
};

#endif // _AUDIO_FILTER_H

#endif //CONFIG_USE_AUDIO
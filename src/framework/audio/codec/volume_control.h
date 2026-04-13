/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _VOLUME_CONTROL_H
#define _VOLUME_CONTROL_H

#include <math.h>

class VolumeControl {
public:
    virtual void ApplyVolume(const int16_t* data, uint32_t samples) 
    {
        int16_t *tmp_data = const_cast<int16_t*>(data);
        for (int i=0; i<samples; i++) {
            tmp_data[i] = Amplify(tmp_data[i]);
        }
    }
    
    virtual void SetVolume(int volume) 
    { 
        volume_ = volume; 
        if (volume_ > max_volume_) volume_ = max_volume_;

        gainF2P6 = (uint8_t)((volume_ * 1.0 / max_volume_) * (1 << 6));
    }

private:
    inline int16_t Amplify(int16_t s) {
      int32_t v = (s * gainF2P6)>>6;
      if (v < -32767) return -32767;
      else if (v > 32767) return 32767;
      else return (int16_t)(v&0xffff);
    }

    int max_volume_ = 100; //default
    int volume_;
    uint8_t gainF2P6;

};

#endif // _VOLUME_CONTROL_H

#endif //
/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO==1

#ifndef _AUDIO_MIXER_OUTPUT_H
#define _AUDIO_MIXER_OUTPUT_H

#include "../audio_output.h"

/**
 * 音频混音输出
 */
class AudioMixerOutput : public AudioOutput {

};

#endif //_AUDIO_MIXER_OUTPUT_H
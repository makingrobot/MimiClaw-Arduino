/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_AUDIO == 1

#ifndef _AUDIO_COMMON_H
#define _AUDIO_COMMON_H

#include <cstdint>
#include <vector>

#define AUDIO_CODEC_DMA_DESC_NUM 6
#define AUDIO_CODEC_DMA_FRAME_NUM 240
#define AUDIO_CODEC_DEFAULT_MIC_GAIN 30.0

/**
 * @enum sample_rate_t
 * @brief Select samples per second
 */
typedef enum
{
    SAMPLE_RATE_8K = 0,        /*!< set to  8k samples per second */
    SAMPLE_RATE_11K = 11025,   /*!< set to 11.025k samples per second */
    SAMPLE_RATE_16K = 16000,   /*!< set to 16k samples in per second */
    SAMPLE_RATE_22K = 22050,   /*!< set to 22.050k samples per second */
    SAMPLE_RATE_24K = 24000,   /*!< set to 24k samples in per second */
    SAMPLE_RATE_32K = 32000,   /*!< set to 32k samples in per second */
    SAMPLE_RATE_44K = 44100,   /*!< set to 44.1k samples per second */
    SAMPLE_RATE_48K = 48000,   /*!< set to 48k samples per second */
    SAMPLE_RATE_64K = 64000,   /*!< set to 64k samples per second */
    SAMPLE_RATE_88K = 88200,   /*!< set to 88.2k samples per second */
    SAMPLE_RATE_96K = 96000,   /*!< set to 96k samples per second */
    SAMPLE_RATE_128K = 128000, /*!< set to 128K samples per second */
    SAMPLE_RATE_176K = 176400, /*!< set to 176.4K samples per second */
    SAMPLE_RATE_192K = 192000, /*!< set to 192k samples per second */
} sample_rate_t;

/**
 * @enum sample_bits_t
 * @brief Select number of bits per sample
 */
typedef enum
{
    SAMPLE_BITS_8 = 8,
    SAMPLE_BITS_16 = 16,
    SAMPLE_BITS_18 = 18,
    SAMPLE_BITS_20 = 20,
    SAMPLE_BITS_24 = 24,
    SAMPLE_BITS_32 = 32,
} sample_bits_t;

/**
 * @enum channels_t
 * @brief Select the number of channels
 */
typedef enum
{
    CHANNELS_1 = 1,
    CHANNELS_2 = 2,
    CHANNELS_4 = 4,
    CHANNELS_8 = 8,
    CHANNELS_16 = 16,
} channels_t;

/**
 * @brief audio config
 */
typedef struct
{
    /* input */
    sample_rate_t input_rate;
    sample_bits_t input_bits;
    channels_t input_channels;

    /* output */
    sample_rate_t output_rate;
    sample_bits_t output_bits;
    channels_t output_channels;
} audio_config_t;

typedef struct
{
    int16_t *samples;
    uint32_t length;
} sample_data_t;

typedef struct
{
    std::vector<int16_t> samples;
    uint32_t length;
} filter_data_t;

enum PipeAction {
    Inited,
    Processing,
    Ended,
    Error = 9,
};

#endif

#endif
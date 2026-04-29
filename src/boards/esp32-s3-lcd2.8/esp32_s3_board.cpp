#include "config.h"
#if BOARD_ESP32_S3_LCD_2_8 == 1

#include "driver/gpio.h"
#include "board_config.h"
#include "esp32_s3_board.h"
#include "src/framework/led/ws2812_led.h"
#include "src/framework/board/onebutton_impl.h"
#include <Arduino.h>
#include <SPI.h>

#if CONFIS_USE_SPIFFS==1
#include <SPIFFS.h>
#elif CONFIG_USE_SDFS==1
#include <SD_MMC.h>
#endif

#if CONFIG_USE_DISPLAY==1 && CONFIG_USE_LVGL==1
#include "src/framework/display/gfx_lvgl_driver.h"
#include "lvgl_log_display.h"
#endif

#if CONFIG_USE_AUDIO==1
#include "src/framework/audio/codec/audio_es8311_codec.h"
#endif

#define TAG "Esp32S3Board"

std::vector<uint8_t> MimiBoard::allow_pins = {
        2, 3, 14, 21
    };

void* create_board() { 
    return new Esp32S3Board();
}

Esp32S3Board::Esp32S3Board() : MimiBoard() {
    Log::Info(TAG, "===== Create Board ...... =====");

    Log::Info(TAG, "initial led.");
    led_ = new Ws2812Led(BUILTIN_LED_PIN, 1);

    InitFileSystem();
    InitButtons();
    InitDisplay();
    InitAudioCodec();

    Log::Info( TAG, "===== Board config completed. =====");
}

void Esp32S3Board::InitButtons() {
    std::shared_ptr<Button> boot_btn = std::make_shared<OneButtonImpl>(kBootButton, BOOT_BUTTON_PIN);
    boot_btn->BindAction(ButtonAction::DoubleClick);
    AddButton(boot_btn);

    btntick_task_ = new FrtTask("btn_tick");
    btntick_task_->OnLoop([this](){
        for (const auto& pair : button_map()) {
            pair.second->Tick();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    });
    btntick_task_->Start(4096, 5);
}

void Esp32S3Board::InitAudioCodec() {
#if CONFIG_USE_AUDIO==1
    // Initialize I2C peripheral
    Log::Info(TAG, "initial i2c.");
    i2c_master_bus_config_t i2c_bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
        },
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));

    Log::Info(TAG, "initial audio codec es8311.");
    /* 使用ES8311 驱动 */
    audio_codec_ = new AudioEs8311Codec(
        i2c_bus_, 
        I2C_NUM_0, 
        AUDIO_INPUT_SAMPLE_RATE, 
        AUDIO_OUTPUT_SAMPLE_RATE,
        AUDIO_I2S_MCLK_PIN, 
        AUDIO_I2S_BCLK_PIN, 
        AUDIO_I2S_LRC_PIN, 
        AUDIO_I2S_DOUT_PIN, 
        AUDIO_I2S_DIN_PIN,
        AUDIO_CODEC_PA_PIN, 
        AUDIO_CODEC_ES8311_ADDR,
        true,
        true);
#endif
}

void Esp32S3Board::InitDisplay() {
#if CONFIG_USE_DISPLAY==1 && CONFIG_USE_LVGL==1
    Log::Info( TAG, "Create GFX driver." );
    gfx_bus_ = new Arduino_ESP32SPI(
        DISPLAY_DC_PIN /* DC */, 
        DISPLAY_CS_PIN /* CS*/, 
        DISPLAY_CLK_PIN /* SCK */, 
        DISPLAY_MOSI_PIN /* MOSI */, 
        DISPLAY_MISO_PIN /* MISO */
    );

    gfx_graphics_ = new Arduino_ILI9341(
        gfx_bus_, 
        DISPLAY_RST_PIN, 
        1 /* rotation */, 
        false /* IPS */);

    if (!gfx_graphics_->begin()) {
        Log::Error(TAG, "gfx-begin() failed!");
        return;
    } 
    gfx_graphics_->invertDisplay(DISPLAY_INVERT_COLOR);
    
    Log::Info( TAG, "Create Lvgl display." );
    disp_driver_ = new GfxLvglDriver(gfx_graphics_, 320, 240);
    display_ = new LvglLogDisplay(disp_driver_, {
                                    .text_font = &font_puhui_16_4,
                                    .icon_font = &font_awesome_16_4,
                                    .emoji_font = font_emoji_32_init(),
                                });

    // backlight_ = new PwmBacklight(DISPLAY_LED_PIN);
    // backlight_->RestoreBrightness();
    pinMode(DISPLAY_LED_PIN, OUTPUT);
    digitalWrite(DISPLAY_LED_PIN, HIGH);
#endif
}

void Esp32S3Board::InitFileSystem() {
    Log::Info( TAG, "Init file system ......" );

#if CONFIS_USE_SPIFFS==1
    if (!SPIFFS.begin(true)) {
        Log::Warn(TAG, "SPIFFS mount failed");
        if(!SPIFFS.format()){
            Log::Error(TAG, "SPIFFS format failed");
            return;
        }
        SPIFFS.begin();
    }

    filesystem_ = new FileSystem(SPIFFS);
    filesystem_->SetTotalBytes(SPIFFS.totalBytes());
    filesystem_->SetFreeBytes(SPIFFS.totalBytes() - SPIFFS.usedBytes());
    filesystem_->SetType("SPIFFS");

#elif CONFIG_USE_SDFS==1
    if (!SD_MMC.setPins(SD_MMC_CLK_PIN, SD_MMC_CMD_PIN, 
            SD_MMC_D1_PIN, SD_MMC_D2_PIN, SD_MMC_D3_PIN, SD_MMC_D4_PIN)) {
        Log::Warn(TAG, "Set SD MMC pin set failed!");
        return;
    }

    if (!SD_MMC.begin()) {
        Log::Warn(TAG, "SD Card mount failed!");
        return;
    }

    filesystem_ = new FileSystem(SD_MMC);
    filesystem_->SetTotalBytes(SD_MMC.totalBytes());
    filesystem_->SetFreeBytes(SD_MMC.totalBytes() - SD_MMC.usedBytes());
    filesystem_->SetType("SDMMCFS");
#endif
}

#endif
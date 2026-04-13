/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "system_reset.h"
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include "log.h"

#define TAG "SystemReset"

SystemReset::SystemReset(gpio_num_t reset_nvs_pin, gpio_num_t reset_factory_pin) : reset_nvs_pin_(reset_nvs_pin), reset_factory_pin_(reset_factory_pin) {
    // Configure GPIO1, GPIO2 as INPUT, reset NVS flash if the button is pressed
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << reset_nvs_pin_) | (1ULL << reset_factory_pin_);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
}


void SystemReset::CheckButtons() {
    if (gpio_get_level(reset_factory_pin_) == 0) {
        Log::Info( TAG, "Button is pressed, reset to factory" );
        ResetNvsFlash();
        ResetToFactory();
    }

    if (gpio_get_level(reset_nvs_pin_) == 0) {
        Log::Info( TAG, "Button is pressed, reset NVS flash" );
        ResetNvsFlash();
    }
}

void SystemReset::ResetNvsFlash() {
    Log::Info( TAG, "Resetting NVS flash" );
    esp_err_t ret = nvs_flash_erase();
    if (ret != ESP_OK) {
        Log::Error( TAG, "Failed to erase NVS flash" );
    }
    ret = nvs_flash_init();
    if (ret != ESP_OK) {
        Log::Error( TAG, "Failed to initialize NVS flash" );
    }
}

void SystemReset::ResetToFactory() {
    Log::Info( TAG, "Resetting to factory" );
    // Erase otadata partition
    const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
    if (partition == NULL) {
        Log::Error( TAG, "Failed to find otadata partition" );
        return;
    }
    esp_partition_erase_range(partition, 0, partition->size);
    Log::Info( TAG, "Erased otadata partition" );

    // Reboot in 3 seconds
    RestartInSeconds(3);
}

void SystemReset::RestartInSeconds(int seconds) {
    for (int i = seconds; i > 0; i--) {
        Log::Info( TAG, "Resetting in %d seconds", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    esp_restart();
}

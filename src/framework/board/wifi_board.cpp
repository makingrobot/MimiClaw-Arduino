/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "config.h"
#if CONFIG_USE_WIFI==1

#include "wifi_board.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cJSON.h>
#include <WiFi.h>
#include <esp_mac.h>

#include "board_def.h"
#include "../sys/log.h"
#include "../wifi/wifi_station.h"
#include "../wifi/ssid_manager.h"
#include "../display/display.h"
#include "../app/application.h"
#include "../sys/system_info.h"
#include "../sys/settings.h"
#include "../lang/lang_zh_cn.h"
#include "../audio/audio_codec.h"

#if CONFIG_WIFI_CONFIGURE_ENABLE==1
#include "../wifi/wifi_configuration.h"
#endif //CONFIG_WIFI_CONFIGURE_ENABLE

#if CONFIG_WIFI_CONFIGURE_WEBSERVER==1
#include "wifi_configuration_impl.h"
#endif

#define TAG "WifiBoard"

WifiBoard::WifiBoard() {
    Settings settings("wifi", true);
    wifi_config_mode_ = settings.GetInt("force_ap") == 1;
    if (wifi_config_mode_) {
        Log::Info( TAG, "force_ap is set to 1, reset to 0" );
        settings.SetInt("force_ap", 0);
    }
}

std::string WifiBoard::GetBoardType() {
    return "wifi";
}

#if CONFIG_WIFI_CONFIGURE_ENABLE==1
void WifiBoard::EnterWifiConfigMode() {
    auto& application = Application::GetInstance();
    application.SetDeviceState(kDeviceStateWifiConfiguring);

    WifiConfiguration *conf = GetWifiConfiguration();
    conf->SetLanguage(Lang::CODE);
    conf->SetSsidPrefix("xpstem");
    conf->Start();

    Log::Info( TAG, "进入 WiFi AP 配置模式。" );
    
    // 显示配置 WiFi 的提示
    application.ShowWifiConfigHit(conf->GetSsid(), conf->GetWebServerUrl(), SystemInfo::GetMacAddress());

    // Wait forever until reset after configuration
    while (true) {
        Log::Info(TAG, "等待配网......" );
        // int free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        // int min_free_sram = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
        // Log::Info(TAG, "Free internal: %u minimal internal: %u", free_sram, min_free_sram);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void WifiBoard::Configure() {
    // User can press BOOT button while starting to enter WiFi configuration mode
    if (wifi_config_mode_) {
        EnterWifiConfigMode();
        return;
    }

    // If no WiFi SSID is configured, enter WiFi configuration mode
    auto& ssid_manager = SsidManager::GetInstance();
    auto ssid_list = ssid_manager.GetSsidList();
    if (ssid_list.empty()) {
        wifi_config_mode_ = true;
        EnterWifiConfigMode();
        return;
    }
}

void WifiBoard::ResetWifiConfiguration() {
    // Set a flag and reboot the device to enter the network configuration mode
    {
        Settings settings("wifi", true);
        settings.SetInt("force_ap", 1);
    }
    GetDisplay()->ShowNotification(Lang::Strings::ENTERING_WIFI_CONFIG_MODE);
    vTaskDelay(pdMS_TO_TICKS(1000));
    // Reboot the device
    Log::Info( TAG, "重启设备......" );
    esp_restart();
}

#endif //CONFIG_WIFI_CONFIGURE_ENABLE

#if CONFIG_WIFI_CONFIGURE_ENABLE==1
WifiConfiguration* WifiBoard::GetWifiConfiguration() {
#if CONFIG_WIFI_CONFIGURE_WEBSERVER==1
    static WifiConfiguration *conf = new WifiConfigurationImpl();
    return conf;
#else
    return nullptr;
#endif
}
#endif

bool WifiBoard::StartAP(const char* ssid, const IPAddress& address, const IPAddress& gateway, const IPAddress& subnet, const char* passwd) {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(address, gateway, subnet);
    if (!WiFi.softAP(ssid, passwd)) {
        Log::Info(TAG, "Soft AP creation failed.");
        return false;
    }
    Log::Info(TAG, "Wifi AP %s on %s", ssid, address.toString());
    return true;
}

bool WifiBoard::StartNetwork(uint32_t timeout_ms) {
    
    auto& wifi_station = WifiStation::GetInstance();
    if (wifi_station.IsConnected()) {
        return true;
    }
    
    wifi_station.OnScanBegin([this]() {
        auto display = Board::GetInstance().GetDisplay();
        display->ShowNotification(Lang::Strings::SCANNING_WIFI, 30000);
    });
    wifi_station.OnConnect([this](const std::string& ssid) {
        auto display = Board::GetInstance().GetDisplay();
        std::string notification = Lang::Strings::CONNECT_TO;
        notification += ssid;
        notification += "...";
        display->ShowNotification(notification.c_str(), 30000);
    });
    wifi_station.OnConnected([this](const std::string& ssid) {
        auto display = Board::GetInstance().GetDisplay();
        std::string notification = Lang::Strings::CONNECTED_TO;
        notification += ssid;
        display->ShowNotification(notification.c_str(), 30000);
    });
    wifi_station.Start();
    Log::Info( TAG, "进入 WiFi STA 联网模式。");

    // Try to connect to WiFi, if failed, launch the WiFi configuration AP
    if (!wifi_station.WaitForConnected(timeout_ms)) {
        wifi_station.Stop();
        wifi_config_mode_ = true;
        // EnterWifiConfigMode();
        return false;
    }

    return true;
}

bool WifiBoard::StartNetwork(const std::string& ssid, const std::string& password, uint32_t timeout_ms) {

    auto& wifi_station = WifiStation::GetInstance();
    if (wifi_station.IsConnected() && wifi_station.GetSsid()==ssid) {
        return true;
    }
    
    wifi_station.OnScanBegin([this]() {
        auto display = Board::GetInstance().GetDisplay();
        display->ShowNotification(Lang::Strings::SCANNING_WIFI, 30000);
    });
    wifi_station.OnConnect([this](const std::string& ssid) {
        auto display = Board::GetInstance().GetDisplay();
        std::string notification = Lang::Strings::CONNECT_TO;
        notification += ssid;
        notification += "...";
        display->ShowNotification(notification.c_str(), 30000);
    });
    wifi_station.OnConnected([this](const std::string& ssid) {
        auto display = Board::GetInstance().GetDisplay();
        std::string notification = Lang::Strings::CONNECTED_TO;
        notification += ssid;
        display->ShowNotification(notification.c_str(), 30000);
    });
    Log::Info( TAG, "进入 WiFi STA 联网模式。");

    // Try to connect to WiFi, if failed, launch the WiFi configuration AP
    if (!wifi_station.WaitForConnected(ssid, password, timeout_ms)) {
        wifi_station.Stop();
        return false;
    }

    return true;
}

void WifiBoard::Disconnect() {
    auto& wifi_station = WifiStation::GetInstance();
    wifi_station.Stop();
}

std::string WifiBoard::GetIpAddress() {
    auto& wifi_station = WifiStation::GetInstance();
    return wifi_station.GetIpAddress();
}

const char* WifiBoard::GetNetworkStateIconName() {
    if (wifi_config_mode_) {
        return "WIFI";
    }
    auto& wifi_station = WifiStation::GetInstance();
    if (!wifi_station.IsConnected()) {
        return "WIFI_OFF";
    }
    int8_t rssi = wifi_station.GetRssi();
    if (rssi >= -60) {
        return "WIFI";
    } else if (rssi >= -70) {
        return "WIFI_FAIR";
    } else {
        return "WIFI_WEAK";
    }
}

std::string WifiBoard::GetBoardJson() {
    // Set the board type for OTA
    auto& wifi_station = WifiStation::GetInstance();
    std::string board_json = R"({)";
    board_json += R"("type":")" + GetBoardType() + R"(",)";
    board_json += R"("name":")" + std::string(BOARD_NAME) + R"(",)";
    if (!wifi_config_mode_) {
        board_json += R"("ssid":")" + wifi_station.GetSsid() + R"(",)";
        board_json += R"("rssi":)" + std::to_string(wifi_station.GetRssi()) + R"(,)";
        board_json += R"("channel":)" + std::to_string(wifi_station.GetChannel()) + R"(,)";
        board_json += R"("ip":")" + wifi_station.GetIpAddress() + R"(",)";
    }
    board_json += R"("mac":")" + SystemInfo::GetMacAddress() + R"(")";
    board_json += R"(})";
    return board_json;
}

void WifiBoard::SetPowerSaveMode(bool enabled) {
    auto& wifi_station = WifiStation::GetInstance();
    wifi_station.SetPowerSaveMode(enabled);
}

std::string WifiBoard::GetDeviceStatusJson() {
    /*
     * 返回设备状态JSON
     * 
     * 返回的JSON结构如下：
     * {
     *     "audio_speaker": {
     *         "volume": 70
     *     },
     *     "screen": {
     *         "brightness": 100,
     *         "theme": "light"
     *     },
     *     "battery": {
     *         "level": 50,
     *         "charging": true
     *     },
     *     "network": {
     *         "type": "wifi",
     *         "ssid": "Xiaozhi",
     *         "rssi": -60
     *     },
     *     "chip": {
     *         "temperature": 25
     *     }
     * }
     */
    auto& board = Board::GetInstance();
    auto root = cJSON_CreateObject();

#if CONFIG_USE_AUDIO==1
    // Audio speaker
    auto audio_speaker = cJSON_CreateObject();
    auto audio_codec = board.GetAudioCodec();
    if (audio_codec) {
        cJSON_AddNumberToObject(audio_speaker, "volume", audio_codec->output_volume());
    }
    cJSON_AddItemToObject(root, "audio_speaker", audio_speaker);
#endif

    // Screen brightness
    auto backlight = board.GetBacklight();
    auto screen = cJSON_CreateObject();
    if (backlight) {
        cJSON_AddNumberToObject(screen, "brightness", backlight->brightness());
    }
    cJSON_AddItemToObject(root, "screen", screen);

    // Battery
    int battery_level = 0;
    bool charging = false;
    bool discharging = false;
    if (board.GetBatteryLevel(battery_level, charging, discharging)) {
        cJSON* battery = cJSON_CreateObject();
        cJSON_AddNumberToObject(battery, "level", battery_level);
        cJSON_AddBoolToObject(battery, "charging", charging);
        cJSON_AddItemToObject(root, "battery", battery);
    }

    // Network
    auto network = cJSON_CreateObject();
    auto& wifi_station = WifiStation::GetInstance();
    cJSON_AddStringToObject(network, "type", "wifi");
    cJSON_AddStringToObject(network, "ssid", wifi_station.GetSsid().c_str());
    int rssi = wifi_station.GetRssi();
    if (rssi >= -60) {
        cJSON_AddStringToObject(network, "signal", "strong");
    } else if (rssi >= -70) {
        cJSON_AddStringToObject(network, "signal", "medium");
    } else {
        cJSON_AddStringToObject(network, "signal", "weak");
    }
    cJSON_AddItemToObject(root, "network", network);

    // Chip
    float esp32temp = 0.0f;
    if (board.GetTemperature(esp32temp)) {
        auto chip = cJSON_CreateObject();
        cJSON_AddNumberToObject(chip, "temperature", esp32temp);
        cJSON_AddItemToObject(root, "chip", chip);
    }

    auto json_str = cJSON_PrintUnformatted(root);
    std::string json(json_str);
    cJSON_free(json_str);
    cJSON_Delete(root);
    return json;
}

#endif
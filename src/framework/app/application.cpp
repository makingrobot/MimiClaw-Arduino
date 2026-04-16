/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "application.h"
#include <stdexcept>

#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_event.h>

#include "config.h"
#include "../display/display.h"
#include "../board/board.h"
#include "../sys/system_info.h"
#include "../lang/lang_zh_cn.h"
#include "../types.h"
#include "../sys/log.h"
#include "../sys/timer.h"
#include "../sys/mutex/std_mutex.h"

#if CONFIG_USE_WIFI==1
#include "../board/wifi_board.h"
#include "../wifi/wifi_station.h"
#endif

#define TAG "Application"

Application::Application() {
    
    // Initialize the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize NVS flash for WiFi configuration
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Log::Info(TAG, "rasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    mutex_ = new StdMutex();
    event_group_ = new FrtEventGroup("main");
}

Application::~Application() {

#if CONFIG_CLOCK_ENABLE==1
    if (clock_timer_!=nullptr) {
        clock_timer_->Stop();
    }
#endif

}

/**
 * 应用初始化
 * 
 * 包括：显示模块初始化、时钟定时器、WiFi配置和连网
 */
void Application::Init() {
    Log::Info(TAG, "Initialize...");
        
    // 显示模块初始化
    auto& board = Board::GetInstance();
    Display* display = board.GetDisplay();
    display->Init();

#if CONFIG_CLOCK_ENABLE==1
    // 时钟定时器，可重载OnClockTimer插入自定义功能
    clock_timer_ = TimerFactory::CreateTimer("Clock");
    clock_timer_->Start(1000, [this](){ OnClockTimer(); });
    Log::Info(TAG, "clock timer started.");
#endif

    SetDeviceState(kDeviceStateStarting);

    display->UpdateStatusBar(true);

#if CONFIG_WIFI_CONFIGURE_ENABLE==1    
    // WiFi配置
    WifiBoard* wifi_board = static_cast<WifiBoard *>(&board);
    if (wifi_board == nullptr) {
        Log::Error(TAG, "不能转换为WifiBoard");
        return;
    }

    wifi_board->Configure();  //阻塞
    
    // 连接热点，默认不连接
    //wifi_board->StartNetwork(30000);
#endif

    SetDeviceState(kDeviceStateIdle);

    eventloop_task_ = new FrtTask("EventLoop_Task");
    eventloop_task_->OnLoop([this](){
        EventLoop();
    });
    eventloop_task_->Start(16384, tskIDLE_PRIORITY + 1);

    if (!OnInit()) {
        while (1) {
            // 死机处理
            Log::Error(TAG, "OnInit failure.");
            delay(1000);
        }
    }
    
    // Print heap stats
    SystemInfo::PrintHeapStats();
    Log::Info(TAG, "Started.");
}

void Application::Loop() {
    OnLoop();
}

void Application::Alert(const char* status, const char* message, const char* emotion) {
    Log::Info(TAG, "Alert %s: %s [%s]", status, message, emotion);
    auto display = Board::GetInstance().GetDisplay();
    display->SetStatus(status);
    display->SetText(message);
}

void Application::DismissAlert() {
    if (device_state_ == kDeviceStateIdle) {
        auto display = Board::GetInstance().GetDisplay();
        display->SetStatus(Lang::Strings::STANDBY);
        display->SetText("");
    }
}

void Application::ToggleWorkState() {
    if (device_state_ == kDeviceStateWorking) {
        SetDeviceState(kDeviceStateIdle);
        return;
    } else if (device_state_ == kDeviceStateIdle) {

        Board& board = Board::GetInstance();
        board.GetBacklight()->RestoreBrightness();

        SetDeviceState(kDeviceStateWorking);
        return;
    }
}

// Add a async task to MainLoop
void Application::Schedule(callback_function_t callback) {
    MutexGuard lock(mutex_);
    if (lock.IsLocked())
    {
        app_tasks_.push_back(std::move(callback));
        event_group_->SetBits(EventHandler::kEventScheduleTask);
    }
}

bool Application::OnPhysicalButtonEvent(const std::string& button_name, const ButtonAction action) {

    if (button_name == kBootButton) {

        if (action == ButtonAction::Click) {
            ToggleWorkState();
            return true;

        } else if (action == ButtonAction::DoubleClick) {
            Board& board = Board::GetInstance();
            board.Shutdown();
            return true;
        }
    }

    // 未处理
    Log::Info(TAG, "%s event unhandle.", button_name.c_str());
    return false;
}

bool Application::OnPinTouchEvent(const std::string& pin_name) {

    // 未处理
    Log::Info(TAG, "%s event unhandle.", pin_name.c_str());
    return false;
}

bool Application::OnDisplayTouchEvent(const TouchPoint_t& point) {
    
    return false;
}

#if CONFIG_CLOCK_ENABLE==1
void Application::OnClockTimer() {
    clock_ticks_++;

    Display* display = Board::GetInstance().GetDisplay();
    display->UpdateStatusBar();

    // Print the debug info every 60 seconds
    if (clock_ticks_ % 60 == 0) {
        Log::Info(TAG, "clock ticks: %d", clock_ticks_);
        // SystemInfo::PrintTaskCpuUsage(pdMS_TO_TICKS(1000));
        // SystemInfo::PrintTaskList();
        SystemInfo::PrintHeapStats();
    }
}
#endif

bool Application::CanEnterSleepMode() {
    if (device_state_ != kDeviceStateIdle) {
        return false;
    }

    return true;
}

void Application::ShowWifiConfigHit(const std::string& ssid, const std::string& config_url, const std::string& mac_address) {
#if CONFIG_WIFI_CONFIGURE_ENABLE==1
    // 显示 WiFi 配置 AP 的 SSID 和 Web 服务器 URL
    std::string hint = Lang::Strings::CONNECT_TO_HOTSPOT;
    hint += ssid;
    hint += Lang::Strings::ACCESS_VIA_BROWSER;
    hint += config_url;
    hint += "\n\n";
    hint += "Mac地址：";
    hint += mac_address;
    hint += "\n";
    
    // 播报配置 WiFi 的提示
    Alert(Lang::Strings::WIFI_CONFIG_MODE, hint.c_str(), "");
#endif
}

void Application::SetDeviceState(const DeviceState* state) {
    if (device_state_ == state) {
        return;
    }
    Log::Info(TAG, "STATE: %s", state->text().c_str());
    
    clock_ticks_ = 0;
    DeviceState* previous_state = device_state_;
    device_state_ = const_cast<DeviceState*>(state);

    auto& board = Board::GetInstance();
    auto display = board.GetDisplay();

    if (state == kDeviceStateUnknown ||
            state == kDeviceStateIdle) {
        display->SetStatus(Lang::Strings::STANDBY);
    } else if (state == kDeviceStateWifiConfiguring) {
        display->SetStatus(Lang::Strings::WIFI_CONFIG_MODE);
    } else if (state == kDeviceStateConnecting) {
        display->SetStatus(Lang::Strings::CONNECTING);
    } else if (state == kDeviceStateWorking) {
        display->SetStatus(Lang::Strings::WORKING);
    } else if (state == kDeviceStateWarning) {
        display->SetStatus(Lang::Strings::WARNING);
    }

    OnStateChanged();
}

void Application::OnStateChanged() {
    Led* led = Board::GetInstance().GetLed();

}

void Application::Reboot() {
    Log::Info(TAG, "Rebooting..." );
    // 收尾工作
    esp_restart();
}

#if CONFIG_OTA_ENABLE==1
void Application::CheckNewVersion() {
    
    auto& board = Board::GetInstance();
    WifiBoard* wifi_board = static_cast<WifiBoard *>(&board);
    wifi_board->StartNetwork(30000);
    
    // Check for new firmware version or get the MQTT broker address
    Ota ota;

    const int MAX_RETRY = 10;
    int retry_count = 0;
    int retry_delay = 10; // 初始重试延迟为10秒

    while (true) {
        //SetDeviceState(kDeviceStateActivating);
        auto display = board.GetDisplay();
        display->SetStatus(Lang::Strings::CHECKING_NEW_VERSION);

        if (!ota.CheckVersion()) {
            retry_count++;
            if (retry_count >= MAX_RETRY) {
                Log::Error(TAG, "Too many retries, exit version check");
                return;
            }

            char buffer[128];
            snprintf(buffer, sizeof(buffer), Lang::Strings::CHECK_NEW_VERSION_FAILED, retry_delay, ota.GetCheckVersionUrl().c_str());
            Alert(Lang::Strings::ERROR, buffer, "sad");

            Log::Warn(TAG, "Check new version failed, retry in %d seconds (%d/%d)", retry_delay, retry_count, MAX_RETRY);
            for (int i = 0; i < retry_delay; i++) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                if (device_state_ == kDeviceStateIdle) {
                    break;
                }
            }
            retry_delay *= 2; // 每次重试后延迟时间翻倍
            continue;
        }
        retry_count = 0;
        retry_delay = 10; // 重置重试延迟时间

        if (ota.HasNewVersion()) {
            Alert(Lang::Strings::OTA_UPGRADE, Lang::Strings::UPGRADING, "happy");

            vTaskDelay(pdMS_TO_TICKS(3000));

            SetDeviceState(kDeviceStateUpgrading);
            
            //display->SetIcon(FONT_AWESOME_DOWNLOAD);
            std::string message = std::string(Lang::Strings::NEW_VERSION) + ota.GetFirmwareVersion();
            display->SetText(message.c_str());

            board.SetPowerSaveMode(false);
            //audio_service_.Stop();
            vTaskDelay(pdMS_TO_TICKS(1000));

            bool upgrade_success = ota.StartUpgrade([display](int progress, size_t speed) {
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "%d%% %uKB/s", progress, speed / 1024);
                display->SetText(buffer);
            });

            if (!upgrade_success) {
                // Upgrade failed, restart audio service and continue running
                Log::Error(TAG, "Firmware upgrade failed, restarting audio service and continuing operation...");
                //audio_service_.Start(); // Restart audio service
                board.SetPowerSaveMode(true); // Restore power save mode
                Alert(Lang::Strings::ERROR, Lang::Strings::UPGRADE_FAILED, "sad");
                vTaskDelay(pdMS_TO_TICKS(3000));
                // Continue to normal operation (don't break, just fall through)
            } else {
                // Upgrade success, reboot immediately
                Log::Info(TAG, "Firmware upgrade successful, rebooting...");
                display->SetText("Upgrade successful, rebooting...");
                vTaskDelay(pdMS_TO_TICKS(1000)); // Brief pause to show message
                Reboot();
                return; // This line will never be reached after reboot
            }
        }

        // No new version, mark the current version as valid
        ota.MarkCurrentVersionValid();
        if (!ota.HasActivationCode() && !ota.HasActivationChallenge()) {
            event_group_->SetBits(EventHandler::kEventNewVersion);
            // Exit the loop if done checking new version
            break;
        }
    }
}
#endif

void Application::EventLoop() {

    if (event_handler_ == nullptr) {
        event_handler_ = new EventHandler();
    }

    // 等待事件位被设置
    auto bits = event_group_->WaitBits(
        event_handler_->GetEventBits(),
        true, /* 自动清除，避免重复响应 */
        false, /* 任一事件位被设置就返回 */
        -1 /* 无限期等待 */
    );

    try {
        if (bits & EventHandler::kEventScheduleTask) {
            MutexGuard lock(mutex_);
            if (lock.IsLocked())
            {
                auto tasks = std::move(app_tasks_);
                for (auto task : tasks) {
                    task();
                }
            }
        } else {
            event_handler_->HandleEvent(bits);
        }
    } catch (const std::exception& e) {
        Log::Error( TAG, "Caught exception: %s", e.what() );
    }

    //delay(1); //1ms
}

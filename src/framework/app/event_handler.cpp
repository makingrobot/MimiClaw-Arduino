/**
 * ESP32-Arduino-Framework
 * Arduino开发环境下适用于ESP32芯片系列开发板的应用开发框架。
 * 
 * Author: Billy Zhang（billy_zh@126.com）
 */
#include "event_handler.h"
#include "application.h"
#include "device_state.h"
#include "../lang/lang_zh_cn.h"

bool EventHandler::HandleEvent(uint32_t event_bits) {
    
    /** 
     * ！！！注意：耗时长的处理不要在此执行，以免阻塞 ！！！
     * 不涉及UI界面操作的事件处理可以试情况单独建FreeRTOS任务处理。
     * 耗时长的事件处理建议做好实现拆分（计算部分、显示部分）
     */
    auto& app = Application::GetInstance();

    if (event_bits & kEventError) {
        app.SetDeviceState(kDeviceStateIdle);
        app.Alert(Lang::Strings::ERROR, app.GetLastErrorMessage().c_str(), "sad");
        return true;
    }

    return false;
}

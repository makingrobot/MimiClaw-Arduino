#include "mimi_application.h"
#include <esp_heap_caps.h>
#include <vector>
#include <string>
#include "src/framework/board/board.h"
#include "src/framework/file/file_system.h"
#include "ArduinoJson.h"
#include "arduino_json_psram.h"
#include "mimi_board.h"

#if CONFIG_USE_DISPLAY==1 && CONFIG_USE_LVGL==1 
#include "src/framework/display/gfx_lvgl_driver.h"
#include "src/framework/display/lvgl_display.h"
#endif

#define TAG "mimiapp"

void* create_application() { 
    return new MimiApplication();
}

MimiApplication::MimiApplication() : _started(false), _outboundTaskHandle(nullptr) {
    
}

bool MimiApplication::OnInit() {
    char buf[32] = {0};
    snprintf(buf, 31, "MimiClaw AI Agent (%s)", GetAppVersion().c_str());
    MIMI_LOGI(TAG, buf);

#if CONFIG_USE_DISPLAY==1
    LvglDisplay* display =  (LvglDisplay*)(Board::GetInstance().GetDisplay());
    display->SetStatus(buf);
    display->SetMessage("system", "系统启动中...");
#endif

    // Print memory info
    MIMI_LOGI(TAG, "Internal free: %d bytes",
              (int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#ifdef BOARD_HAS_PSRAM
    MIMI_LOGI(TAG, "PSRAM free:    %d bytes",
              (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#endif

    Board& board = Board::GetInstance();
    
    // Get FileSystem
    FileSystem *file_system = board.GetFileSystem();
    if (file_system == nullptr) {
        MIMI_LOGE(TAG, __LINE__, "FileSystem mount failed");
        return false;
    }
    MIMI_LOGI(TAG, "fs type: %s, totalbytes: %ld, freebytes: %ld", 
            file_system->type().c_str(), file_system->totalBytes(), file_system->freeBytes());

    std::vector<const char *> dirs = {MIMI_FILE_SKILLS_DIR, MIMI_FILE_SESSION_DIR, MIMI_FILE_MEMORY_DIR, MIMI_FILE_CONFIG_DIR};
    for (const char *dir : dirs) {
        if (!file_system->ExistsFile(dir)) {
            if (!file_system->CreateDir(dir)) {
                MIMI_LOGE(TAG, __LINE__, "Directory %s create failed.", dir);
            }
        }
    }

    // Initialize subsystems
    if (!_prefs.begin(file_system)) {
        MIMI_LOGE(TAG, __LINE__, "Preferences init failed");
        return false;
    }

    if (!_bus.begin()) {
        MIMI_LOGE(TAG, __LINE__, "Bus init failed");
        return false;
    }

    if (!_memory.begin(file_system)) {
        MIMI_LOGE(TAG, __LINE__, "Memory init failed");
        return false;
    }

    if (!_skills.begin(file_system)) {
        MIMI_LOGE(TAG, __LINE__, "Skills init failed");
        return false;
    }

    if (!_session.begin(file_system)) {
        MIMI_LOGE(TAG, __LINE__, "Session init failed");
        return false;
    }

    if (!_wifi.begin(&_prefs)) {
        MIMI_LOGE(TAG, __LINE__, "WiFi init failed");
        return false;
    }

    if (!_onboard.begin(&_prefs)) {
        MIMI_LOGE(TAG, __LINE__, "Onboard init failed");
        return false;
    }

    if (!_proxy.begin(&_prefs)) {
        MIMI_LOGE(TAG, __LINE__, "Proxy init failed");
        return false;
    }

    if (!_telegram.begin(&_bus, &_prefs)) {
        MIMI_LOGE(TAG, __LINE__, "Telegram init failed");
        return false;
    }
    _telegram.setProxy(&_proxy);

    if (!_feishu.begin(&_bus, &_prefs)) {
        MIMI_LOGE(TAG, __LINE__, "Feishu init failed");
        return false;
    }
    _feishu.setProxy(&_proxy);

    if (!_llm.begin(&_prefs)) {
        MIMI_LOGE(TAG, __LINE__, "LLM init failed");
        return false;
    }
    _llm.setProxy(&_proxy);

    if (!_tools.begin(file_system, &_proxy)) {
        MIMI_LOGE(TAG, __LINE__, "Tools init failed");
        return false;
    }

    if (!_cron.begin(&_bus, file_system)) {
        MIMI_LOGE(TAG, __LINE__, "Cron init failed");
        return false;
    }

    if (!_heartbeat.begin(&_bus, file_system)) {
        MIMI_LOGE(TAG, __LINE__, "Heartbeat init failed");
        return false;
    }

    if (!_ws.begin(&_bus)) {
        MIMI_LOGE(TAG, __LINE__, "WS init failed");
        return false;
    }

    if (!_serial_cli.begin(&_bus)) {
        MIMI_LOGE(TAG, __LINE__, "SerialCli init failed");
        return false;
    }

    // Set up context builder dependencies
    _context.setMemory(&_memory);
    _context.setSkills(&_skills);
    _context.setFileSystem(file_system);

    if (!_agent.begin(&_bus, &_llm, &_session, &_tools, &_context)) {
        MIMI_LOGE(TAG, __LINE__, "Agent init failed");
        return false;
    }

    _websearch.init(&_prefs);
    
    _channel_map[_feishu.name()] = &_feishu;
    _channel_map[_telegram.name()] = &_telegram;
    _channel_map[_ws.name()] = &_ws;
    _channel_map[_serial_cli.name()] = &_serial_cli;
    _channel_map[_local.name()] = &_local;

    registerTools();
    installSkills();
    
    MIMI_LOGI(TAG, "All subsystems initialized");

    // start
    bool state = Start();
    if (state) {
        showMessageOnDisplay("system", "服务已就绪, 等待对话...");
    }
    
    return state;
}

void MimiApplication::outboundTask(void* arg) {
    MimiApplication* self = (MimiApplication*)arg;
    MIMI_LOGI(TAG, "Outbound dispatch started");

    while (true) {
        MimiMsg msg;
        if (!self->_bus.popOutbound(&msg, UINT32_MAX)) continue;

        MIMI_LOGI(TAG, "Dispatching response to %s:%s", msg.channel, msg.chat_id);

        self->showMessageOnDisplay("agent", msg.content);

        MimiChannel *ch = self->findChannel(msg.channel);
        if (ch) {
            if (!ch->sendMessage(msg.chat_id, msg.content)) {
                MIMI_LOGE(TAG, __LINE__, "%s send failed for %s", msg.channel, msg.chat_id);
            }
        } else {
            MIMI_LOGW(TAG, "Unknown channel: %s", msg.channel);
        }

        free(msg.content);
    }
}

bool MimiApplication::Start() {
    if (_started) return true;

    if (!_serial_cli.start()) {
        MIMI_LOGW(TAG, "Serial CLI start failed");
    }

    // Connect WiFi
    if (!_wifi.start()) {
        MIMI_LOGW(TAG, "WiFi start failed");
        showMessageOnDisplay("system", "WiFi 启动失败");
        // WiFi信息缺失，启动配置
        _onboard.start();  //阻塞
        return false;
    }

     // 等待连接上
    if (!_wifi.waitConnected()) { //连接不上
        MIMI_LOGW(TAG, "WiFi connection timeout");
        showMessageOnDisplay("system", "WiFi 连接超时，请重启");
        
        // 启动配置
        _onboard.start();  //阻塞
        return false;
    }

    showMessageOnDisplay("system", String("WiFi connected: ") + _wifi.getIP());

    showMessageOnDisplay("system", "Check new version...");
    
    if (_ota.CheckNewVersion()) { //有新版本
        char buf[64] = {0};
        snprintf(buf, 63, "Found new version %s, Upgrading...", _ota.new_version());
        showMessageOnDisplay("system", String(buf));
        if (_ota.Upgrade()) { //升级成功
            showMessageOnDisplay("system", "Upgrade successfully, Restart...");
            MIMI_LOGI(TAG, "Upgrade successfully, Restart...");
            delay(1000);
            ESP.restart();
        }
    }

    if (!_onboard.start(true)) {
        showMessageOnDisplay("system", "Onboard 服务启动失败");
        MIMI_LOGW(TAG, "Onboard start failed");
        return false;
    }

    showMessageOnDisplay("system", String("WiFi 已连接: ") + _wifi.getIP());
    MIMI_LOGI(TAG, "WiFi connected: %s", _wifi.getIP());

    // Start outbound dispatch first
    BaseType_t ok = xTaskCreatePinnedToCore(
        outboundTask, "outbound_task", MIMI_OUTBOUND_STACK,
        this, MIMI_OUTBOUND_PRIO, &_outboundTaskHandle, MIMI_OUTBOUND_CORE);
    if (ok != pdPASS) {
        MIMI_LOGE(TAG, __LINE__, "Failed to create outbound task");
        return false;
    }

    // Start services
    if (!_agent.start()) {
        MIMI_LOGE(TAG, __LINE__, "Agent start failed");
        return false;
    }

    _feishu.start();
    _telegram.start();
    _ws.start();
    
    _cron.start();
    _heartbeat.start();
    
    _started = true;

    MIMI_LOGI(TAG, "All services started!");
    return true;
}

void MimiApplication::OnLoop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// --- Configuration setters ---

void MimiApplication::setWiFiCredentials(const char* ssid, const char* password) {
    _wifi.setCredentials(ssid, password);
}

void MimiApplication::setTelegramToken(const char* token) {
    _telegram.setToken(token);
}

void MimiApplication::setLLMApiKey(const char* key) {
    _llm.setApiKey(key);
}

void MimiApplication::setLLMModel(const char* model) {
    _llm.setModel(model);
}

void MimiApplication::setLLMProvider(const char* provider) {
    _llm.setProvider(provider);
}

void MimiApplication::setLLMApiUrl(const char* url) {
    _llm.setApiUrl(url);
}

void MimiApplication::setProxy(const char* host, uint16_t port, const char* type) {
    _proxy.set(host, port, type);
}

void MimiApplication::clearProxy() {
    _proxy.clear();
}

void MimiApplication::setTimezone(const char* tz) {
    setenv("TZ", tz, 1);
    tzset();
}

bool MimiApplication::isWiFiConnected() {
    return _wifi.isConnected();
}

const char* MimiApplication::getIP() {
    return _wifi.getIP();
}

void MimiApplication::setFeishuCredentials(const char* app_id, const char* app_secret) {
    _feishu.setCredentials(app_id, app_secret);
}

void MimiApplication::setBraveKey(const char* key) {
    _websearch.setBraveKey(key);
}

void MimiApplication::setTavilyKey(const char* key) {
    _websearch.setTavilyKey(key);
}

void MimiApplication::setSearchProvider(const char* provider) {
    _websearch.setProvider(provider);
}

bool MimiApplication::heartbeatTrigger() {
    return _heartbeat.trigger();
}

void MimiApplication::registerTool(const MimiTool* tool) {
    _tools.registerTool(tool);
}

void MimiApplication::registerTools() {
    std::vector<const MimiTool*>& cron_tools = _cron.tools();
    for (const MimiTool *tool : cron_tools) {
        _tools.registerTool(tool);
    }

    std::vector<const MimiTool*>& ws_tools = _websearch.tools();
    for (const MimiTool *tool : ws_tools) {
        _tools.registerTool(tool);
    }

    MimiBoard *board = (MimiBoard *)(&Board::GetInstance());
    for (const MimiTool *tool : board->tools()) {
        _tools.registerTool(tool);
    }
}

void MimiApplication::installSkills() {
    MimiBoard *board = (MimiBoard *)(&Board::GetInstance());
    for (const SkillInfo *info : board->skills()) {
        _skills.installSkill(info);
    }
}

MimiChannel* MimiApplication::findChannel(const char* name) {
    std::string key = std::string(name);
    auto it = _channel_map.find(key);
    return it != _channel_map.end() ? it->second : nullptr; 
}


/**
 * 取前几行
 */
static String _get_first_lines(const String& text, uint8_t num) {
    std::istringstream iss(text.c_str());
    std::string line;
    std::vector<String> lines;
    while (std::getline(iss, line) && lines.size() < num) {
        String s = String(line.c_str());
        s.trim();
        if (s != "") lines.push_back(s);
    }
    
    String result;
    uint8_t line_num = lines.size();
    if (line_num==1) {
        return lines.at(0);
    }
    
    for (uint8_t i=0; i<line_num-1; i++) {
        if (i>0) { result += "    "; } //第二行开始缩进
        result += lines.at(i) + "\n";
    }
    result += lines.at(line_num-1) + "..."; //最后一行加省略号

    return result;
}

void MimiApplication::showMessageOnDisplay(const String& kind, const String& text) {
#if CONFIG_USE_DISPLAY==1 && CONFIG_USE_LVGL==1
    Schedule([kind, text](){  //同步到 Setup调用 线程上，且按序执行
        LvglDisplay* display =  (LvglDisplay*)(Board::GetInstance().GetDisplay());
        display->SetMessage(kind, _get_first_lines(text, 3));
    });
#endif
}


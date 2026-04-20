#include "mimi_application.h"
#include <esp_heap_caps.h>
#include "src/framework/board/board.h"
#include "src/framework/file/file_system.h"
#include "ArduinoJson.h"
#include "arduino_json_psram.h"
#include "mimi_board.h"

#define TAG "mimiapp"

void* create_application() { 
    return new MimiApplication();
}

MimiApplication::MimiApplication() : _started(false), _outboundTaskHandle(nullptr) {}

bool MimiApplication::OnInit() {
    MIMI_LOGI(TAG, "========================================");
    MIMI_LOGI(TAG, "  MimiClaw - ESP32-S3 AI Agent");
    MIMI_LOGI(TAG, "========================================");

    // Print memory info
    MIMI_LOGI(TAG, "Internal free: %d bytes",
              (int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#ifdef BOARD_HAS_PSRAM
    MIMI_LOGI(TAG, "PSRAM free:    %d bytes",
              (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#endif

    // Get FileSystem
    FileSystem *file_system = Board::GetInstance().GetFileSystem();
    if (file_system == nullptr) {
        MIMI_LOGE(TAG, __LINE__, "FileSystem mount failed");
        return false;
    }
    MIMI_LOGI(TAG, "fs type: %s, totalbytes: %ld, freebytes: %ld", 
            file_system->type().c_str(), file_system->totalBytes(), file_system->freeBytes());

    // Initialize subsystems
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

    if (!_wifi.begin()) {
        MIMI_LOGE(TAG, __LINE__, "WiFi init failed");
        return false;
    }

    if (!_proxy.begin()) {
        MIMI_LOGE(TAG, __LINE__, "Proxy init failed");
        return false;
    }

    if (!_telegram.begin(&_bus)) {
        MIMI_LOGE(TAG, __LINE__, "Telegram init failed");
        return false;
    }
    _telegram.setProxy(&_proxy);

    if (!_feishu.begin(&_bus)) {
        MIMI_LOGE(TAG, __LINE__, "Feishu init failed");
        return false;
    }
    _feishu.setProxy(&_proxy);

    if (!_llm.begin()) {
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

    // Set up context builder dependencies
    _context.setMemory(&_memory);
    _context.setSkills(&_skills);
    _context.setFileSystem(file_system);

    if (!_agent.begin(&_bus, &_llm, &_session, &_tools, &_context)) {
        MIMI_LOGE(TAG, __LINE__, "Agent init failed");
        return false;
    }

    _websearch.init();
    
    registerTools();
    installSkills();
    
    MIMI_LOGI(TAG, "All subsystems initialized");

    // start
    bool state = Start();

    return state;
}

void MimiApplication::outboundTask(void* arg) {
    MimiApplication* self = (MimiApplication*)arg;
    MIMI_LOGI(TAG, "Outbound dispatch started");

    while (true) {
        MimiMsg msg;
        if (!self->_bus.popOutbound(&msg, UINT32_MAX)) continue;

        MIMI_LOGI(TAG, "Dispatching response to %s:%s", msg.channel, msg.chat_id);

        if (strcmp(msg.channel, MIMI_CHAN_TELEGRAM) == 0) {
            if (!self->_telegram.sendMessage(msg.chat_id, msg.content)) {
                MIMI_LOGE(TAG, __LINE__, "Telegram send failed for %s", msg.chat_id);
            } else {
                MIMI_LOGI(TAG, "Telegram send success for %s (%d bytes)",
                          msg.chat_id, (int)strlen(msg.content));
            }

        } else if (strcmp(msg.channel, MIMI_CHAN_WEBSOCKET) == 0) {
            if (!self->_ws.send(msg.chat_id, msg.content)) {
                MIMI_LOGW(TAG, "WS send failed for %s", msg.chat_id);
            }

        } else if (strcmp(msg.channel, MIMI_CHAN_CLI) == 0) {
            if (!self->_serial_cli.sendMessage(msg.chat_id, msg.content)) {
                MIMI_LOGI(TAG, "CLI send failed for [%s]", msg.chat_id);
            }

        } else if (strcmp(msg.channel, MIMI_CHAN_FEISHU) == 0) {
            if (!self->_feishu.sendMessage(msg.chat_id, msg.content)) {
                MIMI_LOGE(TAG, __LINE__, "Feishu send failed for %s", msg.chat_id);
            } else {
                MIMI_LOGI(TAG, "Feishu send success for %s (%d bytes)",
                          msg.chat_id, (int)strlen(msg.content));
            }
            
        } else if (strcmp(msg.channel, MIMI_CHAN_SYSTEM) == 0) {
            MIMI_LOGI(TAG, "System message [%s]: %.128s", msg.chat_id, msg.content);

        } else {
            MIMI_LOGW(TAG, "Unknown channel: %s", msg.channel);
        }

        free(msg.content);
    }
}

bool MimiApplication::Start() {
    if (_started) return true;

    // Connect WiFi
    if (!_wifi.start()) {
        MIMI_LOGW(TAG, "WiFi start failed");
        // 启动配置
        _onboard.start();  //阻塞
        return false;
    }

     // 等待连接上
    if (!_wifi.waitConnected()) { //连接不上
        MIMI_LOGW(TAG, "WiFi connection timeout");
        
        // 启动配置
        _onboard.start();  //阻塞
        return false;
    }

    if (!_onboard.start(true)) {
        MIMI_LOGW(TAG, "Onboard start failed");
        return false;
    }

    MIMI_LOGI(TAG, "WiFi connected: %s", _wifi.getIP());

    // Start outbound dispatch first
    BaseType_t ok = xTaskCreatePinnedToCore(
        outboundTask, "outbound", MIMI_OUTBOUND_STACK,
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
    //_telegram.start();
    _cron.start();
    _heartbeat.start();
    _ws.start();
    _serial_cli.start();
    
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

bool MimiApplication::pushMessage(const MimiMsg* msg) {
    return _bus.pushInbound(msg);
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
#include "mimi_application.h"
#include <esp_heap_caps.h>
#include "src/framework/board/board.h"
#include "src/framework/file/file_system.h"

#define TAG "mimi"

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
        MIMI_LOGE(TAG, "FileSystem mount failed");
        return false;
    }
    MIMI_LOGI(TAG, "fs type: %s, totalbytes: %ld, freebytes: %ld", 
            file_system->type().c_str(), file_system->totalBytes(), file_system->freeBytes());

    // Initialize subsystems
    if (!_bus.begin()) {
        MIMI_LOGE(TAG, "Bus init failed");
        return false;
    }

    if (!_memory.begin(file_system)) {
        MIMI_LOGE(TAG, "Memory init failed");
        return false;
    }

    if (!_skills.begin(file_system)) {
        MIMI_LOGE(TAG, "Skills init failed");
        return false;
    }

    if (!_session.begin(file_system)) {
        MIMI_LOGE(TAG, "Session init failed");
        return false;
    }

    if (!_wifi.begin()) {
        MIMI_LOGE(TAG, "WiFi init failed");
        return false;
    }

    if (!_proxy.begin()) {
        MIMI_LOGE(TAG, "Proxy init failed");
        return false;
    }

    if (!_telegram.begin(&_bus)) {
        MIMI_LOGE(TAG, "Telegram init failed");
        return false;
    }
    _telegram.setProxy(&_proxy);

    if (!_feishu.begin(&_bus)) {
        MIMI_LOGE(TAG, "Feishu init failed");
        return false;
    }
    _feishu.setProxy(&_proxy);

    if (!_llm.begin()) {
        MIMI_LOGE(TAG, "LLM init failed");
        return false;
    }
    _llm.setProxy(&_proxy);

    if (!_tools.begin(file_system, &_proxy)) {
        MIMI_LOGE(TAG, "Tools init failed");
        return false;
    }

    if (!_cron.begin(&_bus, file_system)) {
        MIMI_LOGE(TAG, "Cron init failed");
        return false;
    }

    if (!_heartbeat.begin(&_bus, file_system)) {
        MIMI_LOGE(TAG, "Heartbeat init failed");
        return false;
    }

    if (!_ws.begin(&_bus)) {
        MIMI_LOGE(TAG, "WS init failed");
        return false;
    }

    // Set up context builder dependencies
    _context.setMemory(&_memory);
    _context.setSkills(&_skills);
    _context.setFileSystem(file_system);

    if (!_agent.begin(&_bus, &_llm, &_session, &_tools, &_context)) {
        MIMI_LOGE(TAG, "Agent init failed");
        return false;
    }

    _websearch.init();
    
    MIMI_LOGI(TAG, "All subsystems initialized");

    // use mimi_secrets.h config.
    setWiFiCredentials(MIMI_WIFI_SSID, MIMI_WIFI_PASS);

    setFeishuCredentials(MIMI_FEISHU_APP_ID, MIMI_FEISHU_APP_SECRET);

    // LLM model
    setLLMProvider("openai");
    setLLMApiUrl(MIMI_OPENAI_API_URL);
    setLLMModel(MIMI_OPENAI_MODEL);
    setLLMApiKey(MIMI_OPENAI_API_KEY);

    // 添加工具
    _cron.addTools(&_tools);
    _websearch.addTools(&_tools);

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
                MIMI_LOGE(TAG, "Telegram send failed for %s", msg.chat_id);
            } else {
                MIMI_LOGI(TAG, "Telegram send success for %s (%d bytes)",
                          msg.chat_id, (int)strlen(msg.content));
            }

        } else if (strcmp(msg.channel, MIMI_CHAN_WEBSOCKET) == 0) {
            if (!self->_ws.send(msg.chat_id, msg.content)) {
                MIMI_LOGW(TAG, "WS send failed for %s", msg.chat_id);
            }

        } else if (strcmp(msg.channel, MIMI_CHAN_SYSTEM) == 0) {
            MIMI_LOGI(TAG, "System message [%s]: %.128s", msg.chat_id, msg.content);

        } else if (strcmp(msg.channel, MIMI_CHAN_FEISHU) == 0) {
            if (!self->_feishu.sendMessage(msg.chat_id, msg.content)) {
                MIMI_LOGE(TAG, "Feishu send failed for %s", msg.chat_id);
            } else {
                MIMI_LOGI(TAG, "Feishu send success for %s (%d bytes)",
                          msg.chat_id, (int)strlen(msg.content));
            }
            
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
        return false;
    }

    if (!_wifi.waitConnected(30000)) {
        MIMI_LOGW(TAG, "WiFi connection timeout");
        return false;
    }

    MIMI_LOGI(TAG, "WiFi connected: %s", _wifi.getIP());

    // Start outbound dispatch first
    BaseType_t ok = xTaskCreatePinnedToCore(
        outboundTask, "outbound", MIMI_OUTBOUND_STACK,
        this, MIMI_OUTBOUND_PRIO, &_outboundTaskHandle, MIMI_OUTBOUND_CORE);
    if (ok != pdPASS) {
        MIMI_LOGE(TAG, "Failed to create outbound task");
        return false;
    }

    // Start services
    if (!_agent.start()) {
        MIMI_LOGE(TAG, "Agent start failed");
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

void MimiApplication::setSearchKey(const char* key) {
    _websearch.setKey(key);
}

bool MimiApplication::pushMessage(const MimiMsg* msg) {
    return _bus.pushInbound(msg);
}

void MimiApplication::registerTool(const MimiTool* tool) {
    _tools.registerTool(tool);
}

bool MimiApplication::heartbeatTrigger() {
    return _heartbeat.trigger();
}

bool MimiApplication::searchWeb(const char* query, char* output, size_t output_size) {
    return _websearch.search(query, output, output_size);
}
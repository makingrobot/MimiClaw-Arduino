#include "mimi_ws.h"
#include "mimi_config.h"
#include <ArduinoJson.h>
#include "arduino_json_psram.h"

static const char* TAG MIMI_TAG_UNUSED = "ws";

MimiWS* MimiWS::_instance = nullptr;

MimiWS::MimiWS() : _bus(nullptr), _server(nullptr), _taskHandle(nullptr) {
    memset(_clients, 0, sizeof(_clients));
    _instance = this;
}

bool MimiWS::begin(MimiBus* bus) {
    _bus = bus;
    return true;
}

MimiWS::WsClient* MimiWS::findByNum(uint8_t num) {
    for (int i = 0; i < MIMI_WS_MAX_CLIENTS; i++) {
        if (_clients[i].active && _clients[i].num == num) return &_clients[i];
    }
    return nullptr;
}

MimiWS::WsClient* MimiWS::findByChatId(const char* chatId) {
    for (int i = 0; i < MIMI_WS_MAX_CLIENTS; i++) {
        if (_clients[i].active && strcmp(_clients[i].chat_id, chatId) == 0) return &_clients[i];
    }
    return nullptr;
}

MimiWS::WsClient* MimiWS::addClient(uint8_t num) {
    for (int i = 0; i < MIMI_WS_MAX_CLIENTS; i++) {
        if (!_clients[i].active) {
            _clients[i].active = true;
            _clients[i].num = num;
            snprintf(_clients[i].chat_id, sizeof(_clients[i].chat_id), "ws_%d", num);
            MIMI_LOGI(TAG, "Client connected: %s", _clients[i].chat_id);
            return &_clients[i];
        }
    }
    MIMI_LOGW(TAG, "Max clients reached");
    return nullptr;
}

void MimiWS::removeClient(uint8_t num) {
    for (int i = 0; i < MIMI_WS_MAX_CLIENTS; i++) {
        if (_clients[i].active && _clients[i].num == num) {
            MIMI_LOGI(TAG, "Client disconnected: %s", _clients[i].chat_id);
            _clients[i].active = false;
            return;
        }
    }
}

void MimiWS::eventCallback(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (_instance) _instance->onEvent(num, type, payload, length);
}

void MimiWS::onEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            addClient(num);
            break;

        case WStype_DISCONNECTED:
            removeClient(num);
            break;

        case WStype_TEXT: {
            JsonDocument doc(&spiram_allocator);
            if (deserializeJson(doc, payload, length)) {
                MIMI_LOGW(TAG, "Invalid JSON from ws_%d", num);
                break;
            }

            const char* msgType = doc["type"] | "";
            const char* content = doc["content"] | (const char*)nullptr;

            if (strcmp(msgType, "message") == 0 && content) {
                WsClient* client = findByNum(num);
                const char* chatId = client ? client->chat_id : "ws_unknown";

                // Update chat_id if provided
                const char* reqChatId = doc["chat_id"] | (const char*)nullptr;
                if (reqChatId && client) {
                    strncpy(client->chat_id, reqChatId, sizeof(client->chat_id) - 1);
                    chatId = client->chat_id;
                }

                MIMI_LOGI(TAG, "WS message from %s: %.40s...", chatId, content);

                MimiMsg msg;
                memset(&msg, 0, sizeof(msg));
                strncpy(msg.channel, MIMI_CHAN_WEBSOCKET, sizeof(msg.channel) - 1);
                strncpy(msg.chat_id, chatId, sizeof(msg.chat_id) - 1);
                msg.content = strdup(content);
                if (msg.content && _bus) {
                    if (!_bus->pushInbound(&msg)) {
                        free(msg.content);
                    }
                }
            }
            break;
        }

        default:
            break;
    }
}

bool MimiWS::send(const char* chat_id, const char* text) {
    if (!_server) return false;

    WsClient* client = findByChatId(chat_id);
    if (!client) {
        MIMI_LOGW(TAG, "No WS client with chat_id=%s", chat_id);
        return false;
    }

    JsonDocument doc(&spiram_allocator);
    doc["type"] = "response";
    doc["content"] = text;
    doc["chat_id"] = chat_id;

    String json;
    serializeJson(doc, json);

    _server->sendTXT(client->num, json);
    return true;
}

void MimiWS::wsTask(void* arg) {
    MimiWS* self = (MimiWS*)arg;
    MIMI_LOGI(TAG, "WebSocket server task started on port %d", MIMI_WS_PORT);
    while (true) {
        self->_server->loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool MimiWS::start() {
    if (_server) return true;

    _server = new WebSocketsServer(MIMI_WS_PORT);
    if (!_server) return false;

    _server->begin();
    _server->onEvent(eventCallback);

    MIMI_LOGI(TAG, "WebSocket server started on port %d", MIMI_WS_PORT);

    // Create a task to run the WS loop
    BaseType_t ok = xTaskCreate(wsTask, "ws_srv", 4096, this, 4, &_taskHandle);
    if (ok != pdPASS) {
        MIMI_LOGE(TAG, __LINE__, "Failed to create WS task");
        return false;
    }

    return true;
}

void MimiWS::stop() {
    if (_taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
    if (_server) {
        _server->close();
        delete _server;
        _server = nullptr;
        MIMI_LOGI(TAG, "WebSocket server stopped");
    }
}

void MimiWS::loop() {
    if (_server) _server->loop();
}

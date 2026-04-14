/*
 * MimiClaw - Feishu Bot (Long Polling)
 */
#ifndef MIMI_FEISHU_H
#define MIMI_FEISHU_H

#include <Arduino.h>
#include "mimi_bus.h"
#include "cJSON.h"
#include "esp_websocket_client.h"

class MimiProxy; // forward

/* ── Feishu WS frame (protobuf) ────────────────────────────── */
typedef struct {
    char key[32];
    char value[128];
} ws_header_t;

typedef struct {
    uint64_t seq_id;
    uint64_t log_id;
    int32_t service;
    int32_t method;
    ws_header_t headers[16];
    size_t header_count;
    const uint8_t *payload;
    size_t payload_len;
} ws_frame_t;

class MimiFeishu {
public:
    MimiFeishu();
    bool begin(MimiBus* bus);
    bool start();

    void setCredentials(const char *app_id, const char *app_secret);
    void setProxy(MimiProxy* proxy);

    bool sendMessage(const char* chat_id, const char* text);

    void handleWsFrame(const uint8_t *buf, size_t len);
    void setWsConnected(bool connected) { _ws_connected = connected; }

private:
    char _app_id[64];
    char _app_secret[128];

    MimiBus* _bus = nullptr;
    MimiProxy* _proxy = nullptr;
    TaskHandle_t _taskHandle = nullptr;

    static uint64_t fnv1a64(const char* s);
    String apiCall(const char *url, const char* method, const char* postData = nullptr);
    String httpCall(const char *url, const char* method, const char* postData = nullptr, int timeout_ms=5000);
    void pollTask();
    
    bool dedupCheckAndRecord(const char *message_id);
    bool getTenantToken();
    bool pullWsConfig();
    void processWsEvent(const char *json, size_t len);
    void handleMessage(cJSON *event);
    bool replyMessage(const char *message_id, const char *text);
    int sendWsFrame(const ws_frame_t *f, const uint8_t *payload, size_t payload_len, int timeout_ms);

    /* ── Feishu WebSocket state ────────────────────────────────── */
    esp_websocket_client_handle_t _ws_client = NULL;
    char _ws_url[512] = {0};
    int _ws_ping_interval_ms = 120000;
    int _ws_reconnect_interval_ms = 30000;
    int _ws_reconnect_nonce_ms = 30000;
    int _ws_service_id = 0;
    bool _ws_connected = false;

    /* ── Credentials & token state ─────────────────────────────── */
    char _tenant_token[512] = {0};
    int64_t _token_expire_time = 0;

};

#endif // MIMI_FEISHU_H

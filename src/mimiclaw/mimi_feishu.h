/*
 * MimiClaw-Arduino - Feishu Bot (Long Polling)
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_FEISHU_H
#define MIMI_FEISHU_H

#include <Arduino.h>
#include "mimi_bus.h"
#include "mimi_prefs.h"
#include "esp_websocket_client.h"
#include <ArduinoJson.h>
#include "feishu_pack.h"

class MimiProxy; // forward

class MimiFeishu {
public:
    MimiFeishu();
    bool begin(MimiBus* bus, MimiPrefs *prefs);
    bool start();

    void setCredentials(const char *app_id, const char *app_secret);
    void setProxy(MimiProxy* proxy);

    bool sendMessage(const char* chat_id, const char* text);

    void setWsConnected(bool connected) { _ws_connected = connected; }
    void handleWsFrame(const uint8_t *buf, size_t len);

private:
    String _app_id;
    String _app_secret;

    MimiBus* _bus = nullptr;
    MimiPrefs* _prefs = nullptr;
    MimiProxy* _proxy = nullptr;
    TaskHandle_t _taskHandle = nullptr;

    static uint64_t fnv1a64(const char* s);
    String apiCall(const char *url, const char* method, const char* postData = nullptr);
    String httpCall(const char *url, const char* method, const char* postData = nullptr, int timeout_ms=5000);
    void pollTask();
    
    bool dedupCheckAndRecord(const char *message_id);
    bool getTenantToken();
    bool pullWsConfig();

    void handleWsEvent(const char *json, size_t len);
    void handleMessage(const JsonObject& event_obj);
    
    bool replyMessage(const char *message_id, const char *text);
    int sendWsFrame(const ws_frame_t *f, const uint8_t *payload, size_t payload_len, int timeout_ms);

    /* ── Feishu WebSocket state ────────────────────────────────── */
    esp_websocket_client_handle_t _ws_client = NULL;
    String _ws_url;
    int _ws_ping_interval_ms = 120000;
    int _ws_reconnect_interval_ms = 30000;
    int _ws_reconnect_nonce_ms = 30000;
    int _ws_service_id = 0;
    bool _ws_connected = false;

    /* ── Credentials & token state ─────────────────────────────── */
    String _tenant_token;
    int64_t _token_expire_time = 0;

};

#endif // MIMI_FEISHU_H

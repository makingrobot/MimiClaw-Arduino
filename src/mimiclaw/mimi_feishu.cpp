#include "mimi_feishu.h"
#include "mimi_config.h"
#include "mimi_proxy.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_crt_bundle.h"
#include "esp_timer.h"
#include "esp_event.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "arduino_json_psram.h"
#include "feishu_pack.h"

static const char *TAG = "feishu";

/* ── Feishu API endpoints ──────────────────────────────────── */
#define FEISHU_API_BASE         "https://open.feishu.cn/open-apis"
#define FEISHU_AUTH_URL         FEISHU_API_BASE "/auth/v3/tenant_access_token/internal"
#define FEISHU_SEND_MSG_URL     FEISHU_API_BASE "/im/v1/messages"
#define FEISHU_REPLY_MSG_URL    FEISHU_API_BASE "/im/v1/messages/%s/reply"
#define FEISHU_WS_CONFIG_URL    "https://open.feishu.cn/callback/ws/endpoint"

/* ── Message deduplication ─────────────────────────────────── */
#define FEISHU_DEDUP_CACHE_SIZE 64

static uint64_t s_seen_msg_keys[FEISHU_DEDUP_CACHE_SIZE] = {0};
static size_t s_seen_msg_idx = 0;

MimiFeishu::MimiFeishu() {
    
}

bool MimiFeishu::begin(MimiBus *bus, MimiPrefs *prefs)
{
    _bus = bus;
    _prefs = prefs;

    // Load token from Preferences
    _appId = prefs->getString(MIMI_PREF_FS, MIMI_PREF_FS_APPID, "");
    _appSecret = prefs->getString(MIMI_PREF_FS, MIMI_PREF_FS_APPSECRET, "");

    if (_appId.isEmpty()) _appId = MIMI_FEISHU_APP_ID;
    if (_appSecret.isEmpty()) _appSecret = MIMI_FEISHU_APP_SECRET;

    if (_appId.isEmpty() || _appSecret.isEmpty()) {
        MIMI_LOGW(TAG, "No credentials. Use CLI: set_feishu_creds <APP_ID> <APP_SECRET>");
    } else {
        MIMI_LOGI(TAG, "credentials loaded (app_id=%.8s...)", _appId.c_str());
    }

    return true;
}

void MimiFeishu::setCredentials(const char *app_id, const char *app_secret)
{
    _appId = String(app_id);
    _appSecret = String(app_secret);

    _prefs->putString(MIMI_PREF_FS, MIMI_PREF_FS_APPID, app_id);
    _prefs->putString(MIMI_PREF_FS, MIMI_PREF_FS_APPSECRET, app_secret);
    _prefs->update();
    
    MIMI_LOGI(TAG, "credentials saved");
}

void MimiFeishu::setProxy(MimiProxy* proxy) {
    _proxy = proxy;
}

uint64_t MimiFeishu::fnv1a64(const char *s)
{
    uint64_t h = 1469598103934665603ULL;
    if (!s) return h;
    while (*s) {
        h ^= (unsigned char)(*s++);
        h *= 1099511628211ULL;
    }
    return h;
}

bool MimiFeishu::dedupCheckAndRecord(const char *message_id)
{
    uint64_t key = fnv1a64(message_id);
    for (size_t i = 0; i < FEISHU_DEDUP_CACHE_SIZE; i++) {
        if (s_seen_msg_keys[i] == key) return true;
    }
    s_seen_msg_keys[s_seen_msg_idx] = key;
    s_seen_msg_idx = (s_seen_msg_idx + 1) % FEISHU_DEDUP_CACHE_SIZE;
    return false;
}

int MimiFeishu::sendWsFrame(const ws_frame_t *f, const uint8_t *payload, size_t payload_len, int timeout_ms)
{
    MIMI_LOGD(TAG, __LINE__, "call sendWsFrame...");

    uint8_t *out = (uint8_t *)malloc(2048);
    if (!out) {
        MIMI_LOGE(TAG, __LINE__, "Out of memory.\n");
        return -1;
    }

    size_t pos = 0;
    if (!pb_write_tag(out, sizeof(out), &pos, 1, 0) || !pb_write_varint(out, sizeof(out), &pos, f->seq_id)) return -1;
    if (!pb_write_tag(out, sizeof(out), &pos, 2, 0) || !pb_write_varint(out, sizeof(out), &pos, f->log_id)) return -1;
    if (!pb_write_tag(out, sizeof(out), &pos, 3, 0) || !pb_write_varint(out, sizeof(out), &pos, (uint32_t)f->service)) return -1;
    if (!pb_write_tag(out, sizeof(out), &pos, 4, 0) || !pb_write_varint(out, sizeof(out), &pos, (uint32_t)f->method)) return -1;

    for (size_t i = 0; i < f->header_count; i++) {
        uint8_t hb[256];
        size_t hlen = 0;
        if (!ws_encode_header(hb, sizeof(hb), &hlen, f->headers[i].key, f->headers[i].value)) return -1;
        if (!pb_write_bytes(out, sizeof(out), &pos, 5, hb, hlen)) return -1;
    }
    if (payload && payload_len > 0) {
        if (!pb_write_bytes(out, sizeof(out), &pos, 8, payload, payload_len)) return -1;
    }
    int ret = esp_websocket_client_send_bin(_ws_client, (const char *)out, pos, timeout_ms);
    free(out);
    return ret;
}

/* ── Get / refresh tenant access token ─────────────────────── */
bool MimiFeishu::getTenantToken()
{
    if (_appId.isEmpty() || _appSecret.isEmpty()) {
        MIMI_LOGW(TAG, "No credentials configured");
        return false; //ESP_ERR_INVALID_STATE;
    }

    int64_t now = esp_timer_get_time() / 1000000LL;
    if (!_tenant_token.isEmpty() && _token_expire_time > now + 300) {
        return true; //ESP_OK;
    }

    // 请求处理
    JsonDocument request(&spiram_allocator);
    request["app_id"] = _appId.c_str();
    request["app_secret"] = _appSecret.c_str();
    
    String json_str;
    serializeJson(request, json_str);

    String result = httpCall(FEISHU_AUTH_URL, "POST", json_str.c_str(), 10000);
    if (result.isEmpty()) {
        MIMI_LOGE(TAG, __LINE__, "No content response.");
        return false;
    }

    // 响应处理
    JsonDocument response(&spiram_allocator);
    if (deserializeJson(response, result)) {
        MIMI_LOGE(TAG, __LINE__, "Failed to parse token response"); 
        return false; /*ESP_FAIL;*/
    }

    int code = response["code"].as<int>();
    if ( code != 0) {
        MIMI_LOGE(TAG, __LINE__, "Token request failed: code=%d", code);
        return false; //ESP_FAIL;
    }

    if (response.containsKey("tenant_access_token")) {
        const char *token = response["tenant_access_token"].as<const char *>();
        _tenant_token = String(token);

        int expire = response.containsKey("expire") ? response["expire"].as<int>() : 7200;
        _token_expire_time = now + expire - 300;

        MIMI_LOGI(TAG, "Got tenant access token (expires in %ds)", expire );
    }

    return true;
}

/* ── Feishu API call helper ────────────────────────────────── */
String MimiFeishu::apiCall(const char *url, const char *method, const char *post_data)
{
    if (!getTenantToken()) return "";

    HTTPClient http;
    WiFiClientSecure* client = new WiFiClientSecure();
    client->setInsecure(); // Use root CA bundle in production
    
    int timeout = 15000;
    http.setTimeout(timeout);
    http.setConnectTimeout(timeout);

    if (!http.begin(*client, url)) {
        MIMI_LOGE(TAG, __LINE__, "httpclient begin failure.");
        delete client;
        return "";
    }

    String auth_header = "Bearer " + _tenant_token;
    http.addHeader("Authorization", auth_header);

    MIMI_LOGD(TAG, __LINE__, "%s url: %s", method, url);
    //MIMI_LOGD(TAG, "Authheader: %s", auth_header.c_str());

    String result = "";
    uint8_t retry_count = 0;
    uint8_t retry_times = 3;
    while (1) 
    {
        int httpCode;
        if (strcmp(method, "POST")==0) {
            http.addHeader("Content-Type", "application/json; charset=utf-8");
            httpCode = http.POST(post_data);
        } else {
            httpCode = http.GET();
        }

        if (httpCode > 0) {
            result = http.getString();
            break;  // jump out.
        } else {
            MIMI_LOGE(TAG, __LINE__, "HTTP error: %d", httpCode);
            if (retry_count++ > retry_times) { 
                break; //jump out.
            }

            delay(1000 * retry_count);
            MIMI_LOGI(TAG, "trying %s again...", method);
        }
    }

    http.end();
    delete client;
    return result;
}

/* ── Feishu API call helper ────────────────────────────────── */
String MimiFeishu::httpCall(const char *url, const char *method, const char *post_data, int timeout_ms)
{
    HTTPClient http;
    WiFiClientSecure* client = new WiFiClientSecure();
    client->setInsecure(); // Use root CA bundle in production
    
    http.setTimeout(timeout_ms);
    http.setConnectTimeout(timeout_ms);

    if (!http.begin(*client, url)) {
        delete client;
        return "";
    }

    http.addHeader("locale", "zh");

    MIMI_LOGD(TAG, __LINE__, "%s url: %s", method, url);

    String result = "";
    uint8_t retry_count = 0;
    uint8_t retry_times = 3;
    while (1) 
    {
        int httpCode;
        if (strcmp(method, "POST")==0) {
            http.addHeader("Content-Type", "application/json; charset=utf-8");
            httpCode = http.POST(post_data);
        } else {
            httpCode = http.GET();
        }

        if (httpCode > 0) {
            result = http.getString();
            break; // jump out.
        } else {
            MIMI_LOGE(TAG, __LINE__, "HTTP error: %d", httpCode);
            if (retry_count++ > retry_times) { 
                break; //jump out.
            }

            delay(1000 * retry_count);
            MIMI_LOGI(TAG, "trying %s again...", method);
        }
    }

    http.end();
    delete client;
    return result;
}

/* ── WS long connection ────────────────────────────────────── */
static bool parse_query_param(const char *url, const char *key, char *out, size_t out_size)
{
    const char *q = strchr(url, '?');
    if (!q) return false;
    q++;
    size_t key_len = strlen(key);
    while (*q) {
        const char *eq = strchr(q, '=');
        if (!eq) break;
        const char *amp = strchr(eq + 1, '&');
        size_t name_len = (size_t)(eq - q);
        if (name_len == key_len && strncmp(q, key, key_len) == 0) {
            size_t val_len = amp ? (size_t)(amp - (eq + 1)) : strlen(eq + 1);
            size_t n = (val_len < out_size - 1) ? val_len : out_size - 1;
            memcpy(out, eq + 1, n);
            out[n] = '\0';
            return true;
        }
        if (!amp) break;
        q = amp + 1;
    }
    return false;
}

bool MimiFeishu::pullWsConfig()
{
    // 请求处理
    JsonDocument request(&spiram_allocator);
    request["AppID"] = _appId.c_str();
    request["AppSecret"] = _appSecret.c_str();

    String json_str;
    serializeJson(request, json_str);

    String result = httpCall(FEISHU_WS_CONFIG_URL, "POST", json_str.c_str(), 15000);
    if (result.isEmpty()) {
        return false;
    }
    
    // 响应处理
    JsonDocument response(&spiram_allocator);
    if (deserializeJson(response, result)) {
        MIMI_LOGW(TAG, "Failed to parse response JSON");
        return false; //ESP_FAIL;
    }

    int code = response["code"].as<int>();
    const char *url = response["data"]["URL"] | (const char *)nullptr;
    JsonObject ccfg = response["data"]["ClientConfig"];

    if (code != 0 || !url) {
        MIMI_LOGE(TAG, __LINE__, "Invalid WS config response");
        return false; //ESP_FAIL;
    }

    _ws_url = String(url);
    char sid[24] = {0};
    if (parse_query_param(_ws_url.c_str(), "service_id", sid, sizeof(sid))) {
        _ws_service_id = atoi(sid);
    }

    if (ccfg) {
        if (ccfg.containsKey("PingInterval")) _ws_ping_interval_ms = ccfg["PingInterval"].as<int>() * 1000;
        if (ccfg.containsKey("ReconnectInterval")) _ws_reconnect_interval_ms = ccfg["ReconnectInterval"].as<int>() * 1000;
        if (ccfg.containsKey("ReconnectNonce")) _ws_reconnect_nonce_ms = ccfg["ReconnectNonce"].as<int>() * 1000;
    }
    
    MIMI_LOGI(TAG, "WS config ready: service_id=%d ping=%dms", _ws_service_id, _ws_ping_interval_ms);
    return true; //ESP_OK;
}

/**
 * WS包处理 第二步
 */
void MimiFeishu::handleWsEvent(const char *json, size_t len)
{
    MIMI_LOGD(TAG, __LINE__, "step2: handle websocket event...");

    String json_str = String(json, len);
    JsonDocument doc(&spiram_allocator);
    if (deserializeJson(doc, json_str)) {
        MIMI_LOGW(TAG, "Failed to parse ws event JSON");
        return;
    }

    JsonObject event_obj = doc["event"];
    JsonObject header_obj = doc["header"];
    if (!event_obj.isNull() && !header_obj.isNull()) {
        const char *event_type = header_obj["event_type"] | (const char *)nullptr;
        if (event_type && strcmp(event_type, "im.message.receive_v1") == 0) {
            handleMessage(event_obj);
        }
    } else if (!event_obj.isNull()) {
        handleMessage(event_obj);
    }
}

/**
 * Ws包处理第一步
 */
void MimiFeishu::handleWsFrame(const uint8_t *buf, size_t len)
{
    MIMI_LOGD(TAG, __LINE__, "step1: handle websocket frame...");

    ws_frame_t frame;
    if (!pb_parse_frame(buf, len, &frame)) {
        MIMI_LOGW(TAG, "WS frame parse failed");
        return;
    }

    const char *type = frame_header_value(&frame, "type");
    if (frame.method == 0) {
        if (type && strcmp(type, "pong") == 0 && frame.payload && frame.payload_len > 0) {
            String payload_str = String((const char *)frame.payload, frame.payload_len);
            JsonDocument doc(&spiram_allocator);
            if (deserializeJson(doc, payload_str)) {
                MIMI_LOGW(TAG, "Failed to parse payload JSON");
                return;
            }

            if (doc.containsKey("PingInterval")) _ws_ping_interval_ms = doc["PingInterval"].as<int>() * 1000;
        }
        return;
    }
    if (!type || strcmp(type, "event") != 0) return;
    if (!frame.payload || frame.payload_len == 0) return;

    int code = 200;
    handleWsEvent((const char *)frame.payload, frame.payload_len);

    // 发送应答
    char ack[32] = {0};
    int ack_len = snprintf(ack, sizeof(ack), "{\"code\":%d}", code);
    ws_frame_t resp = frame;
    sendWsFrame(&resp, (const uint8_t *)ack, (size_t)ack_len, 1000);
}

/**
 * WS事件处理
 */
static void feishu_ws_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
    //MIMI_LOGD(TAG, __LINE__, "step0: websocket event callback...");

    (void)base;
    MimiFeishu *self = (MimiFeishu*)arg;
    esp_websocket_event_data_t *e = (esp_websocket_event_data_t *)event_data;
    static uint8_t *rx_buf = NULL;
    static size_t rx_cap = 0;
    if (event_id == WEBSOCKET_EVENT_CONNECTED) {
        self->setWsConnected(true);
        MIMI_LOGI(TAG, "Feishu WS connected");
    } else if (event_id == WEBSOCKET_EVENT_DISCONNECTED) {
        self->setWsConnected(false);
        MIMI_LOGW(TAG, "Feishu WS disconnected");
    } else if (event_id == WEBSOCKET_EVENT_DATA) {
        if (e->op_code != WS_TRANSPORT_OPCODES_BINARY) return;
        size_t need = e->payload_offset + e->data_len;
        if (e->payload_offset == 0) {
            if (rx_buf) free(rx_buf);
            rx_cap = (e->payload_len > need) ? e->payload_len : need;
            rx_buf = (uint8_t *)malloc(rx_cap);
            if (!rx_buf) return;
        } else if (!rx_buf || need > rx_cap) {
            return;
        }
        memcpy(rx_buf + e->payload_offset, e->data_ptr, e->data_len);
        if (need >= e->payload_len) {
            self->handleWsFrame(rx_buf, e->payload_len);
            free(rx_buf);
            rx_buf = NULL;
            rx_cap = 0;
        }
    }
}

/* ── Webhook event handler ─────────────────────────────────── */

/*
 * Feishu Event Callback v2 schema:
 * {
 *   "schema": "2.0",
 *   "header": { "event_type": "im.message.receive_v1", "event_id": "...", ... },
 *   "event": {
 *     "sender": { "sender_id": { "open_id": "ou_xxx" }, "sender_type": "user" },
 *     "message": {
 *       "message_id": "om_xxx",
 *       "chat_id": "oc_xxx",
 *       "chat_type": "p2p" | "group",
 *       "message_type": "text",
 *       "content": "{\"text\":\"hello\"}"
 *     }
 *   }
 * }
 *
 * URL verification challenge:
 * { "challenge": "xxx", "token": "yyy", "type": "url_verification" }
 */
/**
 * WS包处理 第三步
 */
void MimiFeishu::handleMessage(const JsonObject& event_obj)
{
    MIMI_LOGD(TAG, __LINE__, "step3: handle message...");

    String str;
    serializeJson(event_obj, str);
    //MIMI_LOGD(TAG, __LINE__, "Message: %s", str.c_str());

    const char *message_id = event_obj["message"]["message_id"] | (const char *)nullptr;
    const char *chat_type  = event_obj["message"]["chat_type"] | (const char *)"p2p";
    const char *msg_type   = event_obj["message"]["message_type"] | (const char *)"text";
    const char *chat_id    = event_obj["message"]["chat_id"] | (const char *)nullptr;
    const char *content    = event_obj["message"]["content"] | (const char *)nullptr;
    
    /* Deduplication */
    if (message_id && dedupCheckAndRecord(message_id)) {
        MIMI_LOGD(TAG, __LINE__, "Duplicate message %s, skipping", message_id);
        return;
    }

    /* Only handle text messages for now */
    if (strcmp(msg_type, "text") != 0) {
        MIMI_LOGI(TAG, "Ignoring non-text message type: %s", msg_type);
        return;
    }

    if (!content) {
        MIMI_LOGI(TAG, "No content.");
        return;
    }

    /* Parse the content JSON to extract text */
    String content_str = String(content);
    JsonDocument content_doc(&spiram_allocator);
    if (deserializeJson(content_doc, content_str)) {
        MIMI_LOGW(TAG, "Failed to parse message content JSON");
        return;
    }

    const char *text = content_doc["text"] | (const char *)nullptr;
    if (!text) {
        MIMI_LOGW(TAG, "No text node.");
        return;
    }

    /* Strip @bot mention prefix if present (Feishu adds @_user_1 for mentions) */
    const char *cleaned = text;
    if (strncmp(cleaned, "@_user_1 ", 9) == 0) {
        cleaned += 9;
    }
    /* Skip leading whitespace */
    while (*cleaned == ' ' || *cleaned == '\n') cleaned++;

    if (cleaned[0] == '\0') {
        MIMI_LOGW(TAG, "No text.");
        return;
    }

    /* Get sender info */
    const char *sender_id = "";
    JsonObject sender_obj = event_obj["sender"];
    if (!sender_obj.isNull() && sender_obj.containsKey("sender_id")) {
        sender_id = sender_obj["sender_id"]["open_id"] | (const char *)"";
    }

    MIMI_LOGI(TAG, "Message from %s in %s(%s): %.60s%s",
             sender_id, chat_id, chat_type, cleaned, strlen(cleaned) > 60 ? "..." : "");

    /* For p2p (DM) chats, use sender open_id as chat_id for session routing.
     * For group chats, use the chat_id (group ID).
     * This matches the moltbot reference pattern where DMs route by sender. */
    const char *route_id = chat_id;
    if (strcmp(chat_type, "p2p") == 0 && sender_id[0]) {
        route_id = sender_id;
    }

    onMessage(route_id, cleaned);
}

/* Webhook mode intentionally disabled: Feishu channel runs in WebSocket mode only. */

void MimiFeishu::pollTask()
{
    MIMI_LOGI(TAG, "Feishu polling task started");

    while (1) {
        if (!pullWsConfig()) {
            MIMI_LOGW(TAG, "No credentials configured, waiting...");
            vTaskDelay(pdMS_TO_TICKS(60000));
            continue;
        }

        esp_websocket_client_config_t ws_cfg = {};
        ws_cfg.uri = _ws_url.c_str();
        ws_cfg.buffer_size = 2048;
        ws_cfg.task_stack = MIMI_FEISHU_POLL_STACK;
        ws_cfg.reconnect_timeout_ms = _ws_reconnect_interval_ms;
        ws_cfg.network_timeout_ms = 10000;
        ws_cfg.disable_auto_reconnect = false;
        ws_cfg.crt_bundle_attach = esp_crt_bundle_attach;

        _ws_client = esp_websocket_client_init(&ws_cfg);
        if (!_ws_client) {
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        esp_websocket_register_events(_ws_client, WEBSOCKET_EVENT_ANY, feishu_ws_event_handler, this);
        esp_websocket_client_start(_ws_client);

        int64_t last_ping = 0;
        while (_ws_client) {
            if (_ws_connected) {
                int64_t now = esp_timer_get_time() / 1000;
                if (now - last_ping >= _ws_ping_interval_ms) {
                    ws_frame_t ping = {0};
                    ping.seq_id = 0;
                    ping.log_id = 0;
                    ping.service = _ws_service_id;
                    ping.method = 0;
                    ping.header_count = 1;
                    strncpy(ping.headers[0].key, "type", sizeof(ping.headers[0].key) - 1);
                    strncpy(ping.headers[0].value, "ping", sizeof(ping.headers[0].value) - 1);
                    sendWsFrame(&ping, NULL, 0, 1000);
                    last_ping = now;
                }
            }
            if (!esp_websocket_client_is_connected(_ws_client) && !_ws_connected) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        esp_websocket_client_stop(_ws_client);
        esp_websocket_client_destroy(_ws_client);
        _ws_client = NULL;
        _ws_connected = false;
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

bool MimiFeishu::start()
{
    if (_taskHandle) return true;

    MIMI_LOGW(TAG, "Create feishu poll task ...");
    BaseType_t ok = xTaskCreatePinnedToCore(
        [](void *arg) {
            MimiFeishu *self = (MimiFeishu *)arg;
            self->pollTask();
        },
        "fs_poll",
        MIMI_FEISHU_POLL_STACK, //注意：过大可能会导致创建失败。
        this,
        MIMI_FEISHU_POLL_PRIO,
        &_taskHandle,
        MIMI_FEISHU_POLL_CORE);

    if (ok != pdPASS) {
        MIMI_LOGW(TAG, "Feishu poll task create failure.");
        return false;
    }
    return true;
}

bool MimiFeishu::sendMessage(const char *chat_id, const char *text)
{
    if (_appId.isEmpty() || _appSecret.isEmpty()) {
        MIMI_LOGW(TAG, "Cannot send: no credentials configured");
        return false;
    }

    /* Determine receive_id_type based on ID prefix */
    const char *id_type = "chat_id";
    if (strncmp(chat_id, "ou_", 3) == 0) {
        id_type = "open_id";
    }

    char url[256] = {0};
    snprintf(url, sizeof(url), "%s?receive_id_type=%s", FEISHU_SEND_MSG_URL, id_type);
    MIMI_LOGI(TAG, "POST url: %s", url);

    size_t text_len = strlen(text);
    size_t offset = 0;
    int all_ok = 1;

    while (offset < text_len) {
        size_t chunk = text_len - offset;
        if (chunk > MIMI_FEISHU_MAX_MSG_LEN) {
            chunk = MIMI_FEISHU_MAX_MSG_LEN;
        }

        char *segment = (char *)malloc(chunk + 1);
        if (!segment) {
            MIMI_LOGE(TAG, __LINE__, "out of memory.");
            return false;
        }
        memcpy(segment, text + offset, chunk);
        segment[chunk] = '\0';

        /* Build content JSON: {"text":"..."} */
        JsonDocument content_obj(&spiram_allocator);
        content_obj["text"] = segment;
        String content_str;
        serializeJson(content_obj, content_str);
        free(segment);

        if (content_str.isEmpty()) { 
            offset += chunk; 
            all_ok = 0; 
            continue; 
        }

        /* Build message body */
        JsonDocument body_obj(&spiram_allocator);
        body_obj["receive_id"] = chat_id;
        body_obj["msg_type"] = "text";
        body_obj["content"] = content_str;

        String json_str;
        serializeJson( body_obj, json_str);

        if (json_str) {
            String resp = apiCall(url, "POST", json_str.c_str());

            if (!resp.isEmpty()) {
                JsonDocument response(&spiram_allocator);
                if (!deserializeJson(response, resp)) {
                    // 解析成功
                    int code = response["code"].as<int>();
                    if (code != 0) {
                        const char *msg = response["msg"] | (const char *)"unknown";
                        MIMI_LOGW(TAG, "Send failed: code=%d, msg=%s", code, msg);
                        all_ok = 0;
                    } else {
                        MIMI_LOGI(TAG, "Sent to %s (%d bytes)", chat_id, (int)chunk);
                    }
                }
            } else {
                MIMI_LOGE(TAG, __LINE__, "Failed to send message chunk");
                all_ok = 0;
            }
        }

        offset += chunk;
    }

    return all_ok;
}

bool MimiFeishu::replyMessage(const char *message_id, const char *text)
{
    if (_appId.isEmpty() || _appSecret.isEmpty()) {
        MIMI_LOGW(TAG, "Cannot send: no credentials configured");
        return false; //ESP_ERR_INVALID_STATE;
    }

    char url[256] = {0};
    snprintf(url, sizeof(url), FEISHU_REPLY_MSG_URL, message_id);

    JsonDocument content_obj(&spiram_allocator);
    content_obj["text"] = text;
    String content_str;
    serializeJson(content_obj, content_str);
    if (content_str.isEmpty()) {
        MIMI_LOGE(TAG, __LINE__, "request content is empty.");
        return false; //ESP_ERR_NO_MEM;
    }

    JsonDocument body_obj(&spiram_allocator);
    body_obj["msg_type"] = "text";
    body_obj["content"] = content_str;

    String json_str;
    serializeJson(body_obj, json_str);
    if (json_str.isEmpty()) {
        MIMI_LOGE(TAG, __LINE__, "Build request json error.");
        return false; //ESP_ERR_NO_MEM;
    }

    String resp = apiCall(url, "POST", json_str.c_str());
    if (resp.isEmpty()) {
        MIMI_LOGE(TAG, __LINE__, "No content response.");
        return false;
    }

    JsonDocument response(&spiram_allocator);
    if (deserializeJson(response, resp)) {
        MIMI_LOGE(TAG, __LINE__, "Parse response json error.");
        return false;
    }

    // 解析成功
    int code = response["code"].as<int>();
    if (code != 0) {
        const char *msg = response["msg"] | (const char *)"unknown";
        MIMI_LOGW(TAG, "Reply failed: code=%d, msg=%s", code, msg);
        return false;
    }
       
    return true;
}

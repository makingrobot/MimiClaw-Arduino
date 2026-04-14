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
#include "cJSON.h"

#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

/**
 * TODO: Refactor  
 * 1. cJSON -> ArduinoJson lib
 * 3. esp_web_socket  -> WebSockets lib
 */ 

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
    memset(_app_id, 0, sizeof(_app_id));
    memset(_app_secret, 0, sizeof(_app_secret));
    memset(_ws_url, 0, sizeof(_ws_url));
    memset(_tenant_token, 0, sizeof(_tenant_token));
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

/* ── HTTP response accumulator ─────────────────────────────── */
typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} http_resp_t;

static bool pb_read_varint(const uint8_t *buf, size_t len, size_t *pos, uint64_t *out)
{
    uint64_t v = 0;
    int shift = 0;
    while (*pos < len && shift <= 63) {
        uint8_t b = buf[(*pos)++];
        v |= ((uint64_t)(b & 0x7F)) << shift;
        if ((b & 0x80) == 0) {
            *out = v;
            return true;
        }
        shift += 7;
    }
    return false;
}

static bool pb_skip_field(const uint8_t *buf, size_t len, size_t *pos, uint8_t wire_type)
{
    uint64_t n = 0;
    switch (wire_type) {
        case 0:
            return pb_read_varint(buf, len, pos, &n);
        case 1:
            if (*pos + 8 > len) return false;
            *pos += 8;
            return true;
        case 2:
            if (!pb_read_varint(buf, len, pos, &n)) return false;
            if (*pos + (size_t)n > len) return false;
            *pos += (size_t)n;
            return true;
        case 5:
            if (*pos + 4 > len) return false;
            *pos += 4;
            return true;
        default:
            return false;
    }
}

static bool pb_parse_header_msg(const uint8_t *buf, size_t len, ws_header_t *h)
{
    memset(h, 0, sizeof(*h));
    size_t pos = 0;
    while (pos < len) {
        uint64_t tag = 0, slen = 0;
        if (!pb_read_varint(buf, len, &pos, &tag)) return false;
        uint32_t field = (uint32_t)(tag >> 3);
        uint8_t wt = (uint8_t)(tag & 0x07);
        if (wt != 2) {
            if (!pb_skip_field(buf, len, &pos, wt)) return false;
            continue;
        }
        if (!pb_read_varint(buf, len, &pos, &slen)) return false;
        if (pos + (size_t)slen > len) return false;
        if (field == 1) {
            size_t n = (slen < sizeof(h->key) - 1) ? (size_t)slen : sizeof(h->key) - 1;
            memcpy(h->key, buf + pos, n);
            h->key[n] = '\0';
        } else if (field == 2) {
            size_t n = (slen < sizeof(h->value) - 1) ? (size_t)slen : sizeof(h->value) - 1;
            memcpy(h->value, buf + pos, n);
            h->value[n] = '\0';
        }
        pos += (size_t)slen;
    }
    return true;
}

static bool pb_parse_frame(const uint8_t *buf, size_t len, ws_frame_t *f)
{
    memset(f, 0, sizeof(*f));
    size_t pos = 0;
    while (pos < len) {
        uint64_t tag = 0, v = 0, blen = 0;
        if (!pb_read_varint(buf, len, &pos, &tag)) return false;
        uint32_t field = (uint32_t)(tag >> 3);
        uint8_t wt = (uint8_t)(tag & 0x07);
        if (field == 1 && wt == 0) {
            if (!pb_read_varint(buf, len, &pos, &f->seq_id)) return false;
        } else if (field == 2 && wt == 0) {
            if (!pb_read_varint(buf, len, &pos, &f->log_id)) return false;
        } else if (field == 3 && wt == 0) {
            if (!pb_read_varint(buf, len, &pos, &v)) return false;
            f->service = (int32_t)v;
        } else if (field == 4 && wt == 0) {
            if (!pb_read_varint(buf, len, &pos, &v)) return false;
            f->method = (int32_t)v;
        } else if (field == 5 && wt == 2) {
            if (!pb_read_varint(buf, len, &pos, &blen)) return false;
            if (pos + (size_t)blen > len) return false;
            if (f->header_count < 16) {
                pb_parse_header_msg(buf + pos, (size_t)blen, &f->headers[f->header_count++]);
            }
            pos += (size_t)blen;
        } else if (field == 8 && wt == 2) {
            if (!pb_read_varint(buf, len, &pos, &blen)) return false;
            if (pos + (size_t)blen > len) return false;
            f->payload = buf + pos;
            f->payload_len = (size_t)blen;
            pos += (size_t)blen;
        } else {
            if (!pb_skip_field(buf, len, &pos, wt)) return false;
        }
    }
    return true;
}

static const char *frame_header_value(const ws_frame_t *f, const char *key)
{
    for (size_t i = 0; i < f->header_count; i++) {
        if (strcmp(f->headers[i].key, key) == 0) {
            return f->headers[i].value;
        }
    }
    return NULL;
}

static bool pb_write_varint(uint8_t *buf, size_t cap, size_t *pos, uint64_t value)
{
    do {
        if (*pos >= cap) return false;
        uint8_t byte = (uint8_t)(value & 0x7F);
        value >>= 7;
        if (value) byte |= 0x80;
        buf[(*pos)++] = byte;
    } while (value);
    return true;
}

static bool pb_write_tag(uint8_t *buf, size_t cap, size_t *pos, uint32_t field, uint8_t wt)
{
    return pb_write_varint(buf, cap, pos, ((uint64_t)field << 3) | wt);
}

static bool pb_write_bytes(uint8_t *buf, size_t cap, size_t *pos, uint32_t field, const uint8_t *data, size_t len)
{
    if (!pb_write_tag(buf, cap, pos, field, 2)) return false;
    if (!pb_write_varint(buf, cap, pos, len)) return false;
    if (*pos + len > cap) return false;
    memcpy(buf + *pos, data, len);
    *pos += len;
    return true;
}

static bool pb_write_string(uint8_t *buf, size_t cap, size_t *pos, uint32_t field, const char *s)
{
    return pb_write_bytes(buf, cap, pos, field, (const uint8_t *)s, strlen(s));
}

static bool ws_encode_header(uint8_t *dst, size_t cap, size_t *out_len, const char *key, const char *value)
{
    size_t pos = 0;
    if (!pb_write_string(dst, cap, &pos, 1, key)) return false;
    if (!pb_write_string(dst, cap, &pos, 2, value)) return false;
    *out_len = pos;
    return true;
}

int MimiFeishu::sendWsFrame(const ws_frame_t *f, const uint8_t *payload, size_t payload_len, int timeout_ms)
{
    uint8_t out[2048];
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
    return esp_websocket_client_send_bin(_ws_client, (const char *)out, pos, timeout_ms);
}

/* ── Get / refresh tenant access token ─────────────────────── */
bool MimiFeishu::getTenantToken()
{
    if (_app_id[0] == '\0' || _app_secret[0] == '\0') {
        MIMI_LOGW(TAG, "No credentials configured");
        return false; //ESP_ERR_INVALID_STATE;
    }

    int64_t now = esp_timer_get_time() / 1000000LL;
    if (_tenant_token[0] != '\0' && _token_expire_time > now + 300) {
        return true; //ESP_OK;
    }

    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "app_id", _app_id);
    cJSON_AddStringToObject(body, "app_secret", _app_secret);
    char *json_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    if (!json_str) return false; //ESP_ERR_NO_MEM;

    String result = httpCall(FEISHU_AUTH_URL, "POST", json_str, 10000);
    free(json_str);
    if (result.isEmpty()) {
        return false;
    }

    cJSON *root = cJSON_Parse(result.c_str());
    if (!root) { MIMI_LOGE(TAG, "Failed to parse token response"); return false; /*ESP_FAIL;*/ }

    cJSON *code = cJSON_GetObjectItem(root, "code");
    if (!code || code->valueint != 0) {
        MIMI_LOGE(TAG, "Token request failed: code=%d", code ? code->valueint : -1);
        cJSON_Delete(root);
        return false; //ESP_FAIL;
    }

    cJSON *token = cJSON_GetObjectItem(root, "tenant_access_token");
    cJSON *expire = cJSON_GetObjectItem(root, "expire");

    if (token && cJSON_IsString(token)) {
        strncpy(_tenant_token, token->valuestring, sizeof(_tenant_token) - 1);
        _token_expire_time = now + (expire ? expire->valueint : 7200) - 300;
        MIMI_LOGI(TAG, "Got tenant access token (expires in %ds)",
                 expire ? expire->valueint : 7200);
    }

    cJSON_Delete(root);
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
        delete client;
        return "";
    }

    char auth_header[600] = {0};
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", _tenant_token);
    http.addHeader("Authorization", auth_header);
    MIMI_LOGI(TAG, "API auth header: %s", auth_header);
    MIMI_LOGI(TAG, "API post data: %s", post_data);

    int httpCode;
    if (post_data) {
        http.addHeader("Content-Type", "application/json; charset=utf-8");
        httpCode = http.POST(post_data);
    } else {
        httpCode = http.GET();
    }

    String result = "";
    if (httpCode > 0) {
        result = http.getString();
    } else {
        MIMI_LOGE(TAG, "HTTP error: %d", httpCode);
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

    int httpCode;
    if (post_data) {
        http.addHeader("Content-Type", "application/json; charset=utf-8");
        httpCode = http.POST(post_data);
    } else {
        httpCode = http.GET();
    }

    String result = "";
    if (httpCode > 0) {
        result = http.getString();
    } else {
        MIMI_LOGE(TAG, "HTTP error: %d", httpCode);
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
    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "AppID", _app_id);
    cJSON_AddStringToObject(body, "AppSecret", _app_secret);
    char *json_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    if (!json_str) return false; //ESP_ERR_NO_MEM;

    String result = httpCall(FEISHU_WS_CONFIG_URL, "POST", json_str, 15000);
    free(json_str);
    if (result.isEmpty()) {
        return false;
    }
    
    cJSON *root = cJSON_Parse(result.c_str());
    if (!root) return false; //ESP_FAIL;

    cJSON *code = cJSON_GetObjectItem(root, "code");
    cJSON *data = cJSON_GetObjectItem(root, "data");
    cJSON *url = data ? cJSON_GetObjectItem(data, "URL") : NULL;
    cJSON *ccfg = data ? cJSON_GetObjectItem(data, "ClientConfig") : NULL;
    if (!code || code->valueint != 0 || !url || !cJSON_IsString(url)) {
        MIMI_LOGE(TAG, "Invalid WS config response");
        cJSON_Delete(root);
        return false; //ESP_FAIL;
    }

    strncpy(_ws_url, url->valuestring, sizeof(_ws_url) - 1);
    char sid[24] = {0};
    if (parse_query_param(_ws_url, "service_id", sid, sizeof(sid))) {
        _ws_service_id = atoi(sid);
    }
    if (ccfg) {
        cJSON *pi = cJSON_GetObjectItem(ccfg, "PingInterval");
        cJSON *ri = cJSON_GetObjectItem(ccfg, "ReconnectInterval");
        cJSON *rn = cJSON_GetObjectItem(ccfg, "ReconnectNonce");
        if (pi && cJSON_IsNumber(pi)) _ws_ping_interval_ms = pi->valueint * 1000;
        if (ri && cJSON_IsNumber(ri)) _ws_reconnect_interval_ms = ri->valueint * 1000;
        if (rn && cJSON_IsNumber(rn)) _ws_reconnect_nonce_ms = rn->valueint * 1000;
    }
    cJSON_Delete(root);
    MIMI_LOGI(TAG, "WS config ready: service_id=%d ping=%dms", _ws_service_id, _ws_ping_interval_ms);
    return true; //ESP_OK;
}

void MimiFeishu::processWsEvent(const char *json, size_t len)
{
    cJSON *root = cJSON_ParseWithLength(json, len);
    if (!root) return;
    cJSON *event = cJSON_GetObjectItem(root, "event");
    cJSON *header = cJSON_GetObjectItem(root, "header");
    if (event && header) {
        cJSON *event_type = cJSON_GetObjectItem(header, "event_type");
        if (event_type && cJSON_IsString(event_type) &&
            strcmp(event_type->valuestring, "im.message.receive_v1") == 0) {
            handleMessage(event);
        }
    } else if (event) {
        handleMessage(event);
    }
    cJSON_Delete(root);
}

void MimiFeishu::handleWsFrame(const uint8_t *buf, size_t len)
{
    ws_frame_t frame = {0};
    if (!pb_parse_frame(buf, len, &frame)) {
        MIMI_LOGW(TAG, "WS frame parse failed");
        return;
    }

    const char *type = frame_header_value(&frame, "type");
    if (frame.method == 0) {
        if (type && strcmp(type, "pong") == 0 && frame.payload && frame.payload_len > 0) {
            cJSON *cfg = cJSON_ParseWithLength((const char *)frame.payload, frame.payload_len);
            if (cfg) {
                cJSON *pi = cJSON_GetObjectItem(cfg, "PingInterval");
                if (pi && cJSON_IsNumber(pi)) _ws_ping_interval_ms = pi->valueint * 1000;
                cJSON_Delete(cfg);
            }
        }
        return;
    }
    if (!type || strcmp(type, "event") != 0) return;
    if (!frame.payload || frame.payload_len == 0) return;

    int code = 200;
    processWsEvent((const char *)frame.payload, frame.payload_len);

    char ack[32];
    int ack_len = snprintf(ack, sizeof(ack), "{\"code\":%d}", code);
    ws_frame_t resp = frame;
    sendWsFrame(&resp, (const uint8_t *)ack, (size_t)ack_len, 1000);
}

static void feishu_ws_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{
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

void MimiFeishu::handleMessage(cJSON *event)
{
    cJSON *message = cJSON_GetObjectItem(event, "message");
    if (!message) return;

    cJSON *message_id_j = cJSON_GetObjectItem(message, "message_id");
    cJSON *chat_id_j    = cJSON_GetObjectItem(message, "chat_id");
    cJSON *chat_type_j  = cJSON_GetObjectItem(message, "chat_type");
    cJSON *msg_type_j   = cJSON_GetObjectItem(message, "message_type");
    cJSON *content_j    = cJSON_GetObjectItem(message, "content");

    if (!chat_id_j || !cJSON_IsString(chat_id_j)) return;
    if (!content_j || !cJSON_IsString(content_j)) return;

    const char *message_id = cJSON_IsString(message_id_j) ? message_id_j->valuestring : "";
    const char *chat_id    = chat_id_j->valuestring;
    const char *chat_type  = cJSON_IsString(chat_type_j) ? chat_type_j->valuestring : "p2p";
    const char *msg_type   = cJSON_IsString(msg_type_j) ? msg_type_j->valuestring : "text";

    /* Deduplication */
    if (message_id[0] && dedupCheckAndRecord(message_id)) {
        MIMI_LOGD(TAG, "Duplicate message %s, skipping", message_id);
        return;
    }

    /* Only handle text messages for now */
    if (strcmp(msg_type, "text") != 0) {
        MIMI_LOGI(TAG, "Ignoring non-text message type: %s", msg_type);
        return;
    }

    /* Parse the content JSON to extract text */
    cJSON *content_obj = cJSON_Parse(content_j->valuestring);
    if (!content_obj) {
        MIMI_LOGW(TAG, "Failed to parse message content JSON");
        return;
    }

    cJSON *text_j = cJSON_GetObjectItem(content_obj, "text");
    if (!text_j || !cJSON_IsString(text_j)) {
        cJSON_Delete(content_obj);
        return;
    }

    const char *text = text_j->valuestring;

    /* Strip @bot mention prefix if present (Feishu adds @_user_1 for mentions) */
    const char *cleaned = text;
    if (strncmp(cleaned, "@_user_1 ", 9) == 0) {
        cleaned += 9;
    }
    /* Skip leading whitespace */
    while (*cleaned == ' ' || *cleaned == '\n') cleaned++;

    if (cleaned[0] == '\0') {
        cJSON_Delete(content_obj);
        return;
    }

    /* Get sender info */
    const char *sender_id = "";
    cJSON *sender = cJSON_GetObjectItem(event, "sender");
    if (sender) {
        cJSON *sender_id_obj = cJSON_GetObjectItem(sender, "sender_id");
        if (sender_id_obj) {
            cJSON *open_id = cJSON_GetObjectItem(sender_id_obj, "open_id");
            if (open_id && cJSON_IsString(open_id)) {
                sender_id = open_id->valuestring;
            }
        }
    }

    MIMI_LOGI(TAG, "Message from %s in %s(%s): %.60s%s",
             sender_id, chat_id, chat_type, cleaned,
             strlen(cleaned) > 60 ? "..." : "");

    /* For p2p (DM) chats, use sender open_id as chat_id for session routing.
     * For group chats, use the chat_id (group ID).
     * This matches the moltbot reference pattern where DMs route by sender. */
    const char *route_id = chat_id;
    if (strcmp(chat_type, "p2p") == 0 && sender_id[0]) {
        route_id = sender_id;
    }

    /* Push to inbound message bus */
    MimiMsg msg;
    strncpy(msg.channel, MIMI_CHAN_FEISHU, sizeof(msg.channel) - 1);
    strncpy(msg.chat_id, route_id, sizeof(msg.chat_id) - 1);
    msg.content = strdup(cleaned);

    if (msg.content) {
        if (!_bus->pushInbound(&msg)) {
            MIMI_LOGW(TAG, "Inbound queue full, dropping message");
            free(msg.content);
        }
    }

    cJSON_Delete(content_obj);
}

/* Webhook mode intentionally disabled: Feishu channel runs in WebSocket mode only. */

/* ── Public API ────────────────────────────────────────────── */

bool MimiFeishu::begin(MimiBus *bus)
{
    _bus = bus;

    // Load token from Preferences
    Preferences prefs;
    if (prefs.begin(MIMI_PREF_FS, true)) {
        String app_id = prefs.getString("app_id", "");
        if (app_id.length() > 0) {
            strncpy(_app_id, app_id.c_str(), sizeof(_app_id) - 1);
        }
        String app_secret = prefs.getString("app_secret", "");
        if (app_secret.length() > 0) {
            strncpy(_app_secret, app_secret.c_str(), sizeof(_app_secret) - 1);
        }
        prefs.end();
    }

    if (_app_id[0] && _app_secret[0]) {
        MIMI_LOGI(TAG, "credentials loaded (app_id=%.8s...)", _app_id);
    } else {
        MIMI_LOGW(TAG, "No credentials. Use CLI: set_feishu_creds <APP_ID> <APP_SECRET>");
    }

    return true;
}


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
        ws_cfg.uri = _ws_url;
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
    if (_app_id[0] == '\0' || _app_secret[0] == '\0') {
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
    MIMI_LOGI(TAG, "Send message to url: %s", url);

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
            MIMI_LOGE(TAG, "out of memory.");
            return false;
        }
        memcpy(segment, text + offset, chunk);
        segment[chunk] = '\0';

        /* Build content JSON: {"text":"..."} */
        cJSON *content = cJSON_CreateObject();
        cJSON_AddStringToObject(content, "text", segment);
        char *content_str = cJSON_PrintUnformatted(content);
        cJSON_Delete(content);
        free(segment);

        if (!content_str) { offset += chunk; all_ok = 0; continue; }

        /* Build message body */
        cJSON *body = cJSON_CreateObject();
        cJSON_AddStringToObject(body, "receive_id", chat_id);
        cJSON_AddStringToObject(body, "msg_type", "text");
        cJSON_AddStringToObject(body, "content", content_str);
        free(content_str);

        char *json_str = cJSON_PrintUnformatted(body);
        cJSON_Delete(body);

        if (json_str) {
            String resp = apiCall(url, "POST", json_str);
            free(json_str);

            if (!resp.isEmpty()) {
                cJSON *root = cJSON_Parse(resp.c_str());
                if (root) {
                    cJSON *code = cJSON_GetObjectItem(root, "code");
                    if (code && code->valueint != 0) {
                        cJSON *msg = cJSON_GetObjectItem(root, "msg");
                        MIMI_LOGW(TAG, "Send failed: code=%d, msg=%s",
                                 code->valueint, msg ? msg->valuestring : "unknown");
                        all_ok = 0;
                    } else {
                        MIMI_LOGI(TAG, "Sent to %s (%d bytes)", chat_id, (int)chunk);
                    }
                    cJSON_Delete(root);
                }
            } else {
                MIMI_LOGE(TAG, "Failed to send message chunk");
                all_ok = 0;
            }
        }

        offset += chunk;
    }

    return all_ok;
}

bool MimiFeishu::replyMessage(const char *message_id, const char *text)
{
    if (_app_id[0] == '\0' || _app_secret[0] == '\0') {
        return false; //ESP_ERR_INVALID_STATE;
    }

    char url[256];
    snprintf(url, sizeof(url), FEISHU_REPLY_MSG_URL, message_id);

    cJSON *content = cJSON_CreateObject();
    cJSON_AddStringToObject(content, "text", text);
    char *content_str = cJSON_PrintUnformatted(content);
    cJSON_Delete(content);
    if (!content_str) return false; //ESP_ERR_NO_MEM;

    cJSON *body = cJSON_CreateObject();
    cJSON_AddStringToObject(body, "msg_type", "text");
    cJSON_AddStringToObject(body, "content", content_str);
    free(content_str);

    char *json_str = cJSON_PrintUnformatted(body);
    cJSON_Delete(body);
    if (!json_str) return ESP_ERR_NO_MEM;

    String resp = apiCall(url, "POST", json_str);
    free(json_str);

    esp_err_t ret = ESP_FAIL;
    if (!resp.isEmpty()) {
        cJSON *root = cJSON_Parse(resp.c_str());
        if (root) {
            cJSON *code = cJSON_GetObjectItem(root, "code");
            if (code && code->valueint == 0) {
                ret = ESP_OK;
            } else {
                cJSON *msg = cJSON_GetObjectItem(root, "msg");
                MIMI_LOGW(TAG, "Reply failed: code=%d, msg=%s",
                         code ? code->valueint : -1, msg ? msg->valuestring : "unknown");
            }
            cJSON_Delete(root);
        }
    }

    return ret == ESP_OK;
}

void MimiFeishu::setCredentials(const char *app_id, const char *app_secret)
{
    strncpy(_app_id, app_id, sizeof(_app_id) - 1);
    strncpy(_app_secret, app_secret, sizeof(_app_secret) - 1);

    Preferences prefs;
    if (prefs.begin(MIMI_PREF_FS, false)) {
        prefs.putString("app_id", app_id);
        prefs.putString("app_secret", app_secret);
        prefs.end();
    }
    MIMI_LOGI(TAG, "credentials saved");
}

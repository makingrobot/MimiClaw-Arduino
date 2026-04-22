#include "mimi_telegram.h"
#include "mimi_config.h"
#include "mimi_proxy.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "arduino_json_psram.h"

static const char* TAG MIMI_TAG_UNUSED = "telegram";

MimiTelegram::MimiTelegram() {
    memset(_token, 0, sizeof(_token));
    memset(_seenKeys, 0, sizeof(_seenKeys));
}

bool MimiTelegram::begin(MimiBus* bus, MimiPrefs* prefs) {
    _bus = bus;
    _prefs = prefs;

    // Load token from Preferences
    String tok = _prefs->getString(MIMI_PREF_TG, MIMI_PREF_TG_TOKEN, MIMI_TG_TOKEN);
    if (tok.length() > 0) {
        strncpy(_token, tok.c_str(), sizeof(_token) - 1);
    }
    _updateOffset = _prefs->getLong64(MIMI_PREF_TG, "offset", 0);
    if (_updateOffset > 0) {
        _lastSavedOffset = _updateOffset;
        MIMI_LOGI(TAG, "Loaded Telegram offset: %lld", (long long)_updateOffset);
    }

    if (_token[0]) {
        MIMI_LOGI(TAG, "Telegram bot token loaded (len=%d)", (int)strlen(_token));
    } else {
        MIMI_LOGW(TAG, "No Telegram bot token configured");
    }
    return true;
}

void MimiTelegram::setToken(const char* token) {
    strncpy(_token, token, sizeof(_token) - 1);
    _prefs->putString(MIMI_PREF_TG, MIMI_PREF_TG_TOKEN, token);
    _prefs->update();
    MIMI_LOGI(TAG, "Telegram bot token saved");
}

void MimiTelegram::setProxy(MimiProxy* proxy) {
    _proxy = proxy;
}

uint64_t MimiTelegram::fnv1a64(const char* s) {
    uint64_t h = 1469598103934665603ULL;
    if (!s) return h;
    while (*s) {
        h ^= (unsigned char)(*s++);
        h *= 1099511628211ULL;
    }
    return h;
}

uint64_t MimiTelegram::makeMsgKey(const char* chatId, int msgId) {
    uint64_t h = fnv1a64(chatId);
    return (h << 16) ^ (uint64_t)(msgId & 0xFFFF) ^ ((uint64_t)msgId << 32);
}

bool MimiTelegram::seenContains(uint64_t key) {
    for (int i = 0; i < DEDUP_CACHE_SIZE; i++) {
        if (_seenKeys[i] == key) return true;
    }
    return false;
}

void MimiTelegram::seenInsert(uint64_t key) {
    _seenKeys[_seenIdx] = key;
    _seenIdx = (_seenIdx + 1) % DEDUP_CACHE_SIZE;
}

void MimiTelegram::saveOffset(bool force) {
    if (_updateOffset <= 0) return;

    unsigned long now = millis();
    bool should = force;
    if (!should && _lastSavedOffset >= 0) {
        if ((_updateOffset - _lastSavedOffset) >= 10) should = true;
        else if ((now - _lastOffsetSaveMs) >= 5000) should = true;
    } else if (!should) {
        should = true;
    }
    if (!should) return;

    _prefs->putLong64(MIMI_PREF_TG, "offset", _updateOffset);
    _prefs->update();
    _lastSavedOffset = _updateOffset;
    _lastOffsetSaveMs = now;
}

void MimiTelegram::loadOffset() {
    _updateOffset = _prefs->getLong64(MIMI_PREF_TG, "offset", 0);
}

String MimiTelegram::apiCall(const char* method, const char* postData) {
    String url = "https://api.telegram.org/bot";
    url += _token;
    url += "/";
    url += method;

    HTTPClient http;
    WiFiClientSecure* client = new WiFiClientSecure();
    client->setInsecure(); // Use root CA bundle in production
    
    int timeout = (MIMI_TG_POLL_TIMEOUT_S + 5) * 1000;
    http.setTimeout(timeout);
    http.setConnectTimeout(timeout);

    if (!http.begin(*client, url)) {
        delete client;
        return "";
    }

    int httpCode;
    if (postData) {
        http.addHeader("Content-Type", "application/json");
        httpCode = http.POST(postData);
    } else {
        httpCode = http.GET();
    }

    String result = "";
    if (httpCode > 0) {
        result = http.getString();
    } else {
        MIMI_LOGE(TAG, __LINE__, "HTTP error: %d", httpCode);
    }

    http.end();
    delete client;
    return result;
}

bool MimiTelegram::responseIsOk(const String& resp) {
    if (resp.length() == 0) return false;

    JsonDocument doc(&spiram_allocator);
    if (deserializeJson(doc, resp) == DeserializationError::Ok) {
        return doc["ok"] | false;
    }

    // Fallback: string check
    return resp.indexOf("\"ok\":true") >= 0;
}

void MimiTelegram::processUpdates(const String& json) {
    JsonDocument doc(&spiram_allocator);
    if (deserializeJson(doc, json)) return;
    if (!(doc["ok"] | false)) return;

    JsonArray results = doc["result"].as<JsonArray>();
    if (results.isNull()) return;

    for (JsonVariant update : results) {
        int64_t uid = (int64_t)(update["update_id"] | (double)-1);
        if (uid >= 0) {
            if (uid < _updateOffset) continue;
            _updateOffset = uid + 1;
            saveOffset(false);
        }

        JsonObject message = update["message"];
        if (message.isNull()) continue;

        const char* text = message["text"] | (const char*)nullptr;
        if (!text) continue;

        JsonObject chat = message["chat"];
        if (chat.isNull()) continue;

        // Get chat_id as string
        char chatIdStr[32] = {0};
        if (chat["id"].is<const char*>()) {
            strncpy(chatIdStr, chat["id"].as<const char*>(), sizeof(chatIdStr) - 1);
        } else {
            snprintf(chatIdStr, sizeof(chatIdStr), "%.0f", (double)chat["id"]);
        }

        int msgIdVal = message["message_id"] | -1;

        // Dedup
        if (msgIdVal >= 0) {
            uint64_t key = makeMsgKey(chatIdStr, msgIdVal);
            if (seenContains(key)) continue;
            seenInsert(key);
        }

        MIMI_LOGI(TAG, "Message from %s: %.40s...", chatIdStr, text);

        // Push to inbound bus
        MimiMsg msg;
        memset(&msg, 0, sizeof(msg));
        strncpy(msg.channel, MIMI_CHAN_TELEGRAM, sizeof(msg.channel) - 1);
        strncpy(msg.chat_id, chatIdStr, sizeof(msg.chat_id) - 1);
        msg.content = strdup(text);
        if (msg.content) {
            if (!_bus->pushInbound(&msg)) {
                MIMI_LOGW(TAG, "Inbound queue full, drop message");
                free(msg.content);
            }
        }
    }
}

bool MimiTelegram::sendChunk(const char* chatId, const char* text, size_t len, bool useMarkdown) {
    JsonDocument doc(&spiram_allocator);
    doc["chat_id"] = chatId;

    // Create segment string
    char* segment = (char*)malloc(len + 1);
    if (!segment) return false;
    memcpy(segment, text, len);
    segment[len] = '\0';
    doc["text"] = segment;
    if (useMarkdown) doc["parse_mode"] = "Markdown";

    String body;
    serializeJson(doc, body);
    free(segment);

    String resp = apiCall("sendMessage", body.c_str());
    return responseIsOk(resp);
}

bool MimiTelegram::sendMessage(const char* chat_id, const char* text) {
    if (_token[0] == '\0') {
        MIMI_LOGW(TAG, "Cannot send: no bot token");
        return false;
    }

    size_t textLen = strlen(text);
    size_t offset = 0;
    bool allOk = true;

    while (offset < textLen) {
        size_t chunk = textLen - offset;
        if (chunk > MIMI_TG_MAX_MSG_LEN) chunk = MIMI_TG_MAX_MSG_LEN;

        MIMI_LOGI(TAG, "Sending chunk to %s (%d bytes)", chat_id, (int)chunk);

        // Try with Markdown first
        bool ok = sendChunk(chat_id, text + offset, chunk, true);
        if (!ok) {
            MIMI_LOGI(TAG, "Markdown rejected, retrying plain");
            ok = sendChunk(chat_id, text + offset, chunk, false);
        }

        if (!ok) {
            MIMI_LOGE(TAG, __LINE__, "Send failed for %s", chat_id);
            allOk = false;
        } else {
            MIMI_LOGI(TAG, "Send success to %s (%d bytes)", chat_id, (int)chunk);
        }

        offset += chunk;
    }

    return allOk;
}

void MimiTelegram::pollTask() {
    MIMI_LOGI(TAG, "Telegram polling task started");

    while (true) {
        if (_token[0] == '\0') {
            MIMI_LOGW(TAG, "No bot token, waiting...");
            vTaskDelay(pdMS_TO_TICKS(60000));
            continue;
        }

        char params[128];
        snprintf(params, sizeof(params), "getUpdates?offset=%lld&timeout=%d",
                 (long long)_updateOffset, MIMI_TG_POLL_TIMEOUT_S);

        String resp = apiCall(params, nullptr);
        if (resp.length() > 0) {
            processUpdates(resp);
        } else {
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
}

bool MimiTelegram::start() {
    if (_taskHandle) return true;

    BaseType_t ok = xTaskCreatePinnedToCore(
        [](void *arg){
            MimiTelegram* self = (MimiTelegram*)arg;
            self->pollTask();
        }, 
        "tg_poll", 
        MIMI_TG_POLL_STACK,  //注意：过大可能会导致创建失败。
        this, 
        MIMI_TG_POLL_PRIO, 
        &_taskHandle, 
        MIMI_TG_POLL_CORE);

    if (ok != pdPASS) {
        MIMI_LOGW(TAG, "Telegram poll task create failure.");
        return false;
    }
    return true;
}

void MimiTelegram::stop() {
    if (_taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
        saveOffset(true);
        MIMI_LOGI(TAG, "Telegram polling stopped");
    }
}

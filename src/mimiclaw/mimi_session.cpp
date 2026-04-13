#include "mimi_session.h"
#include "mimi_config.h"
#include <SPIFFS.h>
#include <time.h>

static const char* TAG MIMI_TAG_UNUSED = "session";

MimiSession::MimiSession() {}

bool MimiSession::begin() {
    MIMI_LOGI(TAG, "Session manager initialized");
    return true;
}

String MimiSession::sessionPath(const char* chat_id) {
    return String(MIMI_SPIFFS_SESSION_DIR) + "/tg_" + chat_id + ".jsonl";
}

bool MimiSession::append(const char* chat_id, const char* role, const char* content) {
    String path = sessionPath(chat_id);
    File f = SPIFFS.open(path, "a");
    if (!f) {
        MIMI_LOGE(TAG, "Cannot open session file %s", path.c_str());
        return false;
    }

    JsonDocument doc;
    doc["role"] = role;
    doc["content"] = content;
    doc["ts"] = (double)time(NULL);

    String line;
    serializeJson(doc, line);
    f.println(line);
    f.close();
    return true;
}

bool MimiSession::getHistoryJson(const char* chat_id, char* buf, size_t size, int maxMsgs) {
    String path = sessionPath(chat_id);
    File f = SPIFFS.open(path, "r");
    if (!f) {
        snprintf(buf, size, "[]");
        return true;
    }

    // Read lines into ring buffer
    struct MsgEntry {
        String role;
        String content;
    };
    
    MsgEntry* msgs = new MsgEntry[maxMsgs];
    int count = 0;
    int writeIdx = 0;

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.isEmpty()) continue;

        JsonDocument lineDoc;
        if (deserializeJson(lineDoc, line)) continue;

        if (count >= maxMsgs) {
            // Ring buffer overwrite
        }
        msgs[writeIdx].role = lineDoc["role"].as<String>();
        msgs[writeIdx].content = lineDoc["content"].as<String>();
        writeIdx = (writeIdx + 1) % maxMsgs;
        if (count < maxMsgs) count++;
    }
    f.close();

    // Build JSON array
    JsonDocument arrDoc;
    JsonArray arr = arrDoc.to<JsonArray>();
    
    int start = (count < maxMsgs) ? 0 : writeIdx;
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % maxMsgs;
        JsonObject entry = arr.add<JsonObject>();
        entry["role"] = msgs[idx].role;
        entry["content"] = msgs[idx].content;
    }

    delete[] msgs;

    String jsonStr;
    serializeJson(arr, jsonStr);
    strncpy(buf, jsonStr.c_str(), size - 1);
    buf[size - 1] = '\0';
    return true;
}

bool MimiSession::getHistoryJson(const char* chat_id, JsonArray& arr, int maxMsgs) {
    String path = sessionPath(chat_id);
    File f = SPIFFS.open(path, "r");
    if (!f) return true; // No history is OK

    struct MsgEntry {
        String role;
        String content;
    };

    MsgEntry* msgs = new MsgEntry[maxMsgs];
    int count = 0;
    int writeIdx = 0;

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.isEmpty()) continue;

        JsonDocument lineDoc;
        if (deserializeJson(lineDoc, line)) continue;

        msgs[writeIdx].role = lineDoc["role"].as<String>();
        msgs[writeIdx].content = lineDoc["content"].as<String>();
        writeIdx = (writeIdx + 1) % maxMsgs;
        if (count < maxMsgs) count++;
    }
    f.close();

    int start = (count < maxMsgs) ? 0 : writeIdx;
    for (int i = 0; i < count; i++) {
        int idx = (start + i) % maxMsgs;
        JsonObject entry = arr.add<JsonObject>();
        entry["role"] = msgs[idx].role;
        entry["content"] = msgs[idx].content;
    }

    delete[] msgs;
    return true;
}

bool MimiSession::clear(const char* chat_id) {
    String path = sessionPath(chat_id);
    if (SPIFFS.remove(path)) {
        MIMI_LOGI(TAG, "Session %s cleared", chat_id);
        return true;
    }
    return false;
}

void MimiSession::list() {
    File root = SPIFFS.open("/");
    if (!root) {
        MIMI_LOGW(TAG, "Cannot open SPIFFS directory");
        return;
    }

    int count = 0;
    File file = root.openNextFile();
    while (file) {
        String name = file.name();
        if (name.indexOf("tg_") >= 0 && name.endsWith(".jsonl")) {
            MIMI_LOGI(TAG, "  Session: %s", name.c_str());
            count++;
        }
        file = root.openNextFile();
    }

    if (count == 0) {
        MIMI_LOGI(TAG, "  No sessions found");
    }
}

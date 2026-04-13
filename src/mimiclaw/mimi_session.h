/*
 * MimiClaw - Session Manager (JSONL files on SPIFFS)
 */
#ifndef MIMI_SESSION_H
#define MIMI_SESSION_H

#include <Arduino.h>
#include <ArduinoJson.h>

class MimiSession {
public:
    MimiSession();
    bool begin();

    bool append(const char* chat_id, const char* role, const char* content);
    bool getHistoryJson(const char* chat_id, char* buf, size_t size, int maxMsgs);
    bool getHistoryJson(const char* chat_id, JsonArray& arr, int maxMsgs);
    bool clear(const char* chat_id);
    void list();

private:
    String sessionPath(const char* chat_id);
};

#endif // MIMI_SESSION_H

/*
 * MimiClaw-Arduino - Session Manager (JSONL files on storage)
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_SESSION_H
#define MIMI_SESSION_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "src/framework/file/file_system.h"

class MimiSession {
public:
    MimiSession();
    bool begin(FileSystem *file_system);

    bool append(const char* chat_id, const char* role, const char* content);
    bool getHistoryJson(const char* chat_id, char* buf, size_t size, int maxMsgs);
    bool getHistoryJson(const char* chat_id, JsonArray& arr, int maxMsgs);
    bool clear(const char* chat_id);
    void list();

private:
    String sessionPath(const char* chat_id);
    FileSystem *_file_system;
};

#endif // MIMI_SESSION_H

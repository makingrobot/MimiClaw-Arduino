/*
 * MimiClaw - Telegram Bot (Long Polling)
 */
#ifndef MIMI_TELEGRAM_H
#define MIMI_TELEGRAM_H

#include <Arduino.h>
#include "mimi_bus.h"

class MimiProxy; // forward

class MimiTelegram {
public:
    MimiTelegram();
    bool begin(MimiBus* bus);
    bool start();
    void stop();

    void setToken(const char* token);
    void setProxy(MimiProxy* proxy);

    bool sendMessage(const char* chat_id, const char* text);

private:
    MimiBus* _bus = nullptr;
    MimiProxy* _proxy = nullptr;
    TaskHandle_t _taskHandle = nullptr;

    char _token[128];
    int64_t _updateOffset = 0;
    int64_t _lastSavedOffset = -1;
    unsigned long _lastOffsetSaveMs = 0;
    
    // Dedup cache
    static const int DEDUP_CACHE_SIZE = 64;
    uint64_t _seenKeys[DEDUP_CACHE_SIZE];
    int _seenIdx = 0;

    static uint64_t fnv1a64(const char* s);
    static uint64_t makeMsgKey(const char* chatId, int msgId);
    bool seenContains(uint64_t key);
    void seenInsert(uint64_t key);

    void saveOffset(bool force = false);
    void loadOffset();

    String apiCall(const char* method, const char* postData = nullptr);
    bool responseIsOk(const String& resp);
    void processUpdates(const String& json);
    bool sendChunk(const char* chatId, const char* text, size_t len, bool useMarkdown);

    void pollTask();
};

#endif // MIMI_TELEGRAM_H

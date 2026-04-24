/*
 * MimiClaw-Arduino - Telegram Bot (Long Polling)
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_TELEGRAM_H
#define MIMI_TELEGRAM_H

#include <Arduino.h>
#include "mimi_bus.h"
#include "mimi_prefs.h"
#include "mimi_channel.h"
#include "mimi_config.h"

class MimiProxy; // forward

class MimiTelegram : public MimiChannel {
public:
    MimiTelegram();
    bool begin(MimiBus* bus, MimiPrefs* prefs);
    bool start();
    void stop();

    void setToken(const char* token);
    void setProxy(MimiProxy* proxy);

    virtual bool sendMessage(const char* chat_id, const char* text) override;

    virtual String name() const override { MIMI_CHAN_TELEGRAM; }

private:
    MimiPrefs* _prefs = nullptr;
    MimiProxy* _proxy = nullptr;
    TaskHandle_t _taskHandle = nullptr;

    String _token;
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

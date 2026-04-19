/*
 * MimiClaw - Message Bus (FreeRTOS queue-based)
 */
#ifndef MIMI_BUS_H
#define MIMI_BUS_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

struct MimiMsg {
    char channel[16];   // "telegram", "feishu", "websocket", "cli", "system"
    char chat_id[96];   // Telegram/Feishu chat_id or WS client id
    char* content;      // Heap-allocated message text (caller must free)
};

class MimiBus {
public:
    MimiBus();
    bool begin();

    bool pushInbound(const MimiMsg* msg);
    bool popInbound(MimiMsg* msg, uint32_t timeout_ms = UINT32_MAX);
    bool pushOutbound(const MimiMsg* msg);
    bool popOutbound(MimiMsg* msg, uint32_t timeout_ms = UINT32_MAX);

private:
    QueueHandle_t _inQueue;
    QueueHandle_t _outQueue;
};

#endif // MIMI_BUS_H

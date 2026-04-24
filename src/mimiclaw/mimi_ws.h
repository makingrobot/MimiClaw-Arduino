/*
 * MimiClaw-Arduino - WebSocket Server
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_WS_H
#define MIMI_WS_H

#include <Arduino.h>
#include <WebSocketsServer.h>
#include "mimi_config.h"
#include "mimi_bus.h"
#include "mimi_channel.h"

class MimiWS : public MimiChannel {
public:
    MimiWS();
    bool begin(MimiBus* bus);
    bool start();
    void stop();
    void loop();   // Must be called periodically if not using a task

    virtual bool sendMessage(const char* chat_id, const char* text) override;

    virtual String name() const override { MIMI_CHAN_WEBSOCKET; }

private:
    WebSocketsServer* _server;
    TaskHandle_t _taskHandle;

    struct WsClient {
        bool active;
        uint8_t num;
        char chat_id[32];
    };
    WsClient _clients[MIMI_WS_MAX_CLIENTS];

    WsClient* findByNum(uint8_t num);
    WsClient* findByChatId(const char* chatId);
    WsClient* addClient(uint8_t num);
    void removeClient(uint8_t num);

    void onEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    static void eventCallback(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    static void wsTask(void* arg);

    static MimiWS* _instance;
};

#endif // MIMI_WS_H

/*
 * MimiClaw-Arduino - Channel
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_CHANNEL_H
#define MIMI_CHANNEL_H

#include <Arduino.h>
#include <string>
#include "mimi_config.h"
#include "mimi_bus.h"

class MimiChannel {
public:
    virtual std::string name() const=0;
    virtual bool sendMessage(const char* chat_id, const char* text)=0;

    virtual bool onMessage(const char* chat_id, const char* text);

protected:
    MimiBus *_bus = nullptr;

};

class MimiLocal : public MimiChannel {
public:
    MimiLocal();
    virtual std::string name() const override { return MIMI_CHAN_SYSTEM; }
    virtual bool sendMessage(const char* chat_id, const char* text) override;
};

#endif // MIMI_CHANNEL_H
/*
 * MimiClaw-Arduino - Serial Cli Console
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_SERIAL_CLI_H
#define MIMI_SERIAL_CLI_H

#include <ESP32Console.h>
#include "mimi_channel.h"
#include "mimi_config.h"

class MimiSerialCli : public MimiChannel {
public:
    MimiSerialCli();
    bool begin(MimiBus* bus);
    bool start();

    virtual bool sendMessage(const char* chat_id, const char* text) override;
    
    virtual std::string name() const override { return MIMI_CHAN_CLI; }

private:
    ESP32Console::Console _console;
};

#endif // MIMI_SERIAL_CLI_H

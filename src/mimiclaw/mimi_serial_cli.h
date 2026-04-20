/*
 * MimiClaw-Arduino - Serial Cli Console
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_SERIAL_CLI_H
#define MIMI_SERIAL_CLI_H

#include <ESP32Console.h>

class MimiSerialCli {
public:
    MimiSerialCli();
    bool start();

    bool sendMessage(const char* chat_id, const char* text);
    
private:
    ESP32Console::Console _console;
};

#endif // MIMI_SERIAL_CLI_H

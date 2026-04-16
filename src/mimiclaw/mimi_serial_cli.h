/*
 * MimiClaw - Serial Cli Console
 */
#ifndef MIMI_SERIAL_CLI_H
#define MIMI_SERIAL_CLI_H

#include <ESP32Console.h>

class MimiSerialCli {
public:
    MimiSerialCli();
    bool start();

private:
    ESP32Console::Console _console;
};

#endif // MIMI_SERIAL_CLI_H

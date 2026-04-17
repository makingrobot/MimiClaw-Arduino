/*
 * MimiClaw - Serial Cli Console
 */
#ifndef MIMI_SERIAL_CLI_H
#define MIMI_SERIAL_CLI_H

#include <ESP32Console.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class MimiSerialCli {
public:
    MimiSerialCli();
    bool start();

    bool sendMessage(const char* chat_id, const char* text);
    QueueHandle_t msg_queue() const { return _msg_queue; }
    
private:
    ESP32Console::Console _console;
    QueueHandle_t _msg_queue;
    TaskHandle_t _task_handle;
};

#endif // MIMI_SERIAL_CLI_H

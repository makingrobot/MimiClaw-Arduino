/*
 * MimiClaw - Heartbeat Service
 */
#ifndef MIMI_HEARTBEAT_H
#define MIMI_HEARTBEAT_H

#include <Arduino.h>
#include "mimi_bus.h"
#include "src/framework/file/file_system.h"

class MimiHeartbeat {
public:
    MimiHeartbeat();
    bool begin(MimiBus* bus, FileSystem *file_system);
    bool start();
    void stop();
    bool trigger();

private:
    MimiBus* _bus;
    TimerHandle_t _timer;
    FileSystem* _file_system;

    bool hasTasks();
    bool sendHeartbeat();

    static void timerCallback(TimerHandle_t xTimer);
};

#endif // MIMI_HEARTBEAT_H

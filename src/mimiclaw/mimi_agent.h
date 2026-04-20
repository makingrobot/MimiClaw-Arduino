/*
 * MimiClaw-Arduino - ReAct Agent Loop
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_AGENT_H
#define MIMI_AGENT_H

#include <Arduino.h>
#include "mimi_bus.h"
#include "mimi_llm.h"
#include "mimi_session.h"
#include "mimi_tools.h"
#include "mimi_context.h"

class MimiAgent {
public:
    MimiAgent();
    bool begin(MimiBus* bus, MimiLLM* llm, MimiSession* session,
               MimiToolRegistry* tools, MimiContext* context);
    bool start();
    void stop();

private:
    MimiBus* _bus;
    MimiLLM* _llm;
    MimiSession* _session;
    MimiToolRegistry* _tools;
    MimiContext* _context;
    TaskHandle_t _taskHandle;

    void processMessage(MimiMsg* msg, char* systemPrompt, char* toolOutput);
    void appendTurnContext(char* prompt, size_t size, const MimiMsg* msg);

    static void agentTask(void* arg);
};

#endif // MIMI_AGENT_H

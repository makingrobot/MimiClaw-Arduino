/*
 * MimiClaw - ESP32-S3 AI Agent Arduino Library
 * 
 * A complete AI agent running on ESP32-S3 with PSRAM.
 * Supports Anthropic Claude and OpenAI APIs, Telegram bot,
 * WebSocket gateway, persistent memory (SPIFFS), cron scheduling,
 * web search, and a ReAct agent loop with tool calling.
 */
#ifndef MIMI_APPLICATION_H
#define MIMI_APPLICATION_H

#include <Arduino.h>
#include "mimi_config.h"
#include "mimi_bus.h"
#include "mimi_wifi.h"
#include "mimi_llm.h"
#include "mimi_agent.h"
#include "mimi_memory.h"
#include "mimi_session.h"
#include "mimi_telegram.h"
#include "mimi_ws.h"
#include "mimi_proxy.h"
#include "mimi_tools.h"
#include "mimi_cron.h"
#include "mimi_heartbeat.h"
#include "mimi_skills.h"
#include "mimi_context.h"
#include "mimi_feishu.h"
#include "src/framework/app/application.h"

class MimiApplication : public Application {
public:
    MimiApplication();
    
    virtual const std::string& GetAppName() const override { return "MimiClaw-Arduino"; };
    virtual const std::string& GetAppVersion() const override { return "1.0.0"; }

    /**
     * Initialize all subsystems. Call in setup().
     * @return true on success
     */
    virtual bool OnInit() override;

    /**
     * Start all services (WiFi, Telegram, Agent, Cron, etc.)
     * Call after begin() and optionally after configuring secrets.
     * @return true on success
     */
    bool Start();

    /**
     * Process loop - call in loop() or let tasks run autonomously.
     * If you don't use FreeRTOS tasks, call this periodically.
     */
    virtual void OnLoop() override;

    // --- Configuration setters (can be called before begin()) ---
    void setWiFi(const char* ssid, const char* password);
    void setTelegramToken(const char* token);
    void setApiKey(const char* key);
    void setModel(const char* model);
    void setModelProvider(const char* provider);  // "anthropic" or "openai"
    void setApiUrl(const char* url);              // Custom API endpoint URL
    void setProxy(const char* host, uint16_t port, const char* type = "http");
    void clearProxy();
    void setSearchKey(const char* key);
    void setTimezone(const char* tz);
    void setFeishuCredentials(const char* app_id, const char* app_secret);

    // --- Status ---
    bool isWiFiConnected();
    const char* getIP();
    
    bool pushMessage(const MimiMsg* msg) {
        return _bus.pushInbound(msg);
    }

    // --- Memory ---
    MimiMemory& memory() { return _memory; }
    MimiSession& session() { return _session; }
    
    // --- Tool registration ---
    MimiToolRegistry& tools() { return _tools; }
    
    // --- Cron ---
    MimiCron& cron() { return _cron; }

private:
    MimiBus _bus;
    MimiWiFi _wifi;
    MimiLLM _llm;
    MimiAgent _agent;
    MimiMemory _memory;
    MimiSession _session;
    MimiTelegram _telegram;
    MimiWS _ws;
    MimiProxy _proxy;
    MimiToolRegistry _tools;
    MimiCron _cron;
    MimiHeartbeat _heartbeat;
    MimiSkills _skills;
    MimiContext _context;
    MimiFeishu _feishu;

    bool _started;
    
    // Outbound dispatch task
    static void outboundTask(void* arg);
    TaskHandle_t _outboundTaskHandle;
};

#endif // MIMI_APPLICATION_H

/*
 * MimiClaw-Arduino
 * 
 * A complete AI agent running on ESP32-S3 with PSRAM.
 * Supports Anthropic Claude and OpenAI APIs, Telegram bot, Feishu bot
 * WebSocket gateway, persistent memory, cron scheduling,
 * web search, and a ReAct agent loop with tool calling.
 *
 * Author: Billy Zhang（billy_zh@126.com）
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
#include "mimi_serial_cli.h"
#include "mimi_onboard.h"
#include "mimi_websearch.h"
#include "mimi_feishu.h"
#include "src/framework/app/application.h"

class MimiApplication : public Application {
public:
    MimiApplication();
    
    virtual const std::string& GetAppName() const override { return "MimiClaw-Arduino"; };
    virtual const std::string& GetAppVersion() const override { return "1.0.0"; }

    virtual bool OnInit() override;
    virtual void OnLoop() override;

    /**
     * Start all services (WiFi, Telegram, Agent, Cron, etc.)
     * Call after begin() and optionally after configuring secrets.
     * @return true on success
     */
    bool Start();

    // --- Configuration setters (can be called before begin()) ---
    void setWiFiCredentials(const char* ssid, const char* password);
    
    void setTelegramToken(const char* token);
    void setFeishuCredentials(const char* app_id, const char* app_secret);

    void setLLMApiKey(const char* key);
    void setLLMModel(const char* model);
    void setLLMProvider(const char* provider);  // "anthropic" or "openai"
    void setLLMApiUrl(const char* url);              // Custom API endpoint URL

    void setProxy(const char* host, uint16_t port, const char* type = "http");
    void clearProxy();
    void setTimezone(const char* tz);
    
    // --- Status ---
    bool isWiFiConnected();
    const char* getIP();

    // 推送消息
    bool pushMessage(const MimiMsg* msg);

    bool heartbeatTrigger();

    // Web搜索
    void setBraveKey(const char* key);
    void setTavilyKey(const char* key);
    void setSearchProvider(const char* provider);

    // 注册工具
    void registerTool(const MimiTool* tool);
    
    // 在显示器上显示文本
    void showOnDisplay(const std::string& text);

    // --- Memory ---
    MimiMemory& memory() { return _memory; }
    MimiSession& session() { return _session; }
    
    // --- Tool registration ---
    MimiToolRegistry& tools() { return _tools; }
    
    // --- Cron ---
    MimiCron& cron() { return _cron; }

    // --- Skills ---
    MimiSkills& skills() { return _skills; }
    
    MimiWebsearch& websearch() { return _websearch; }

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
    MimiSerialCli _serial_cli;
    MimiWebsearch _websearch;
    MimiOnboard _onboard;
    MimiFeishu _feishu;

    bool _started;
    
    // Outbound dispatch task
    static void outboundTask(void* arg);
    
    // 注册工具
    void registerTools();
    // 安装技能
    void installSkills();

    TaskHandle_t _outboundTaskHandle;
};

#endif // MIMI_APPLICATION_H

/*
 * MimiClaw - LLM Proxy (Anthropic / OpenAI)
 */
#ifndef MIMI_LLM_H
#define MIMI_LLM_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "mimi_config.h"

struct LlmToolCall {
    char id[64];
    char name[32];
    char* input;        // heap-allocated JSON string
    size_t input_len;
};

struct LlmResponse {
    char* text;
    size_t text_len;
    LlmToolCall calls[MIMI_MAX_TOOL_CALLS];
    int call_count;
    bool tool_use;
    
    void free_response();
};

class MimiProxy; // forward declaration

class MimiLLM {
public:
    MimiLLM();
    bool begin();

    void setApiKey(const char* key);
    void setModel(const char* model);
    void setProvider(const char* provider);
    void setApiUrl(const char* url);
    void setProxy(MimiProxy* proxy);

    const char* getModel() { return _model; }
    const char* getProvider() { return _provider; }
    const char* getApiUrl() { return _apiUrl; }

    /**
     * Send a chat completion request with tools.
     * @param system_prompt System prompt string
     * @param messages      ArduinoJson array of messages
     * @param tools_json    Pre-built JSON string of tools array, or NULL
     * @param resp          Output response struct
     * @return true on success
     */
    bool chatWithTools(const char* system_prompt, JsonArray messages,
                       const char* tools_json, LlmResponse* resp);

private:
    char _apiKey[320];
    char _model[64];
    char _provider[16];
    char _apiUrl[256];
    MimiProxy* _proxy;
    Preferences _prefs;

    bool isOpenAI();
    const char* apiUrl();
    const char* apiHost();
    const char* apiPath();

    // Build request body
    String buildRequestBody(const char* system_prompt, JsonArray messages, const char* tools_json);
    
    // HTTP call (direct)
    String httpDirect(const String& postData);
    
    // HTTP call via proxy (raw socket)
    String httpViaProxy(const String& postData);
    
    // Parse response
    bool parseAnthropicResponse(const String& body, LlmResponse* resp);
    bool parseOpenAIResponse(const String& body, LlmResponse* resp);
    
    // Convert tools/messages for OpenAI format
    String convertToolsOpenAI(const char* tools_json);
    String convertMessagesOpenAI(const char* system_prompt, JsonArray messages);
};

#endif // MIMI_LLM_H

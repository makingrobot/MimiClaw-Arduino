#include "mimi_llm.h"
#include "mimi_proxy.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "arduino_json_psram.h"

static const char* TAG MIMI_TAG_UNUSED = "llm";

void LlmResponse::free_response() {
    ::free(text);
    text = nullptr;
    text_len = 0;
    for (int i = 0; i < call_count; i++) {
        ::free(calls[i].input);
        calls[i].input = nullptr;
    }
    call_count = 0;
    tool_use = false;
}

MimiLLM::MimiLLM() : _proxy(nullptr) {
    memset(_apiKey, 0, sizeof(_apiKey));
    memset(_apiUrl, 0, sizeof(_apiUrl));
    strncpy(_model, MIMI_LLM_DEFAULT_MODEL, sizeof(_model) - 1);
    strncpy(_provider, MIMI_LLM_PROVIDER_DEFAULT, sizeof(_provider) - 1);
}

bool MimiLLM::begin() {
    // Load from Preferences
    _prefs.begin(MIMI_PREF_LLM, true);
    String key = _prefs.getString("api_key", "");
    String model = _prefs.getString("model", "");
    String provider = _prefs.getString("provider", "");
    String url = _prefs.getString("api_url", "");
    _prefs.end();

    if (!key.isEmpty()) strncpy(_apiKey, key.c_str(), sizeof(_apiKey) - 1);
    if (!model.isEmpty()) strncpy(_model, model.c_str(), sizeof(_model) - 1);
    if (!provider.isEmpty()) strncpy(_provider, provider.c_str(), sizeof(_provider) - 1);
    if (!url.isEmpty()) strncpy(_apiUrl, url.c_str(), sizeof(_apiUrl) - 1);

    if (_apiKey[0]) {
        MIMI_LOGI(TAG, "LLM initialized (provider: %s, model: %s)", _provider, _model);
    } else {
        MIMI_LOGW(TAG, "No API key configured");
    }
    return true;
}

void MimiLLM::setApiKey(const char* key) {
    strncpy(_apiKey, key, sizeof(_apiKey) - 1);
    _prefs.begin(MIMI_PREF_LLM, false);
    _prefs.putString("api_key", key);
    _prefs.end();
    MIMI_LOGI(TAG, "API key saved");
}

void MimiLLM::setModel(const char* model) {
    strncpy(_model, model, sizeof(_model) - 1);
    _prefs.begin(MIMI_PREF_LLM, false);
    _prefs.putString("model", model);
    _prefs.end();
    MIMI_LOGI(TAG, "Model set to: %s", _model);
}

void MimiLLM::setProvider(const char* provider) {
    strncpy(_provider, provider, sizeof(_provider) - 1);
    _prefs.begin(MIMI_PREF_LLM, false);
    _prefs.putString("provider", provider);
    _prefs.end();
    MIMI_LOGI(TAG, "Provider set to: %s", _provider);
}

void MimiLLM::setApiUrl(const char* url) {
    strncpy(_apiUrl, url, sizeof(_apiUrl) - 1);
    _prefs.begin(MIMI_PREF_LLM, false);
    _prefs.putString("api_url", url);
    _prefs.end();
    MIMI_LOGI(TAG, "API URL set to: %s", _apiUrl);
}

void MimiLLM::setProxy(MimiProxy* proxy) {
    _proxy = proxy;
}

bool MimiLLM::isOpenAI() {
    return strcmp(_provider, "openai") == 0;
}

const char* MimiLLM::apiUrl() {
    if (_apiUrl[0]) return _apiUrl;
    return isOpenAI() ? MIMI_OPENAI_API_URL : MIMI_LLM_API_URL;
}

const char* MimiLLM::apiHost() {
    // Parse host from custom URL if set
    const char* url = apiUrl();
    const char* p = strstr(url, "://");
    if (p) p += 3; else p = url;
    static char host[128];
    const char* end = strchr(p, '/');
    size_t len = end ? (size_t)(end - p) : strlen(p);
    // Strip port number
    const char* colon = (const char*)memchr(p, ':', len);
    if (colon) len = (size_t)(colon - p);
    if (len >= sizeof(host)) len = sizeof(host) - 1;
    memcpy(host, p, len);
    host[len] = '\0';
    return host;
}

const char* MimiLLM::apiPath() {
    const char* url = apiUrl();
    const char* p = strstr(url, "://");
    if (p) p += 3; else p = url;
    const char* slash = strchr(p, '/');
    return slash ? slash : "/";
}

String MimiLLM::buildRequestBody(const char* system_prompt, JsonArray messages, const char* tools_json) {
    JsonDocument doc(&spiram_allocator);
    doc["model"] = _model;

    if (isOpenAI()) {
        doc["max_completion_tokens"] = MIMI_LLM_MAX_TOKENS;

        // Build OpenAI messages with system prompt as first message
        JsonArray msgs = doc["messages"].to<JsonArray>();
        if (system_prompt && system_prompt[0]) {
            JsonObject sys = msgs.add<JsonObject>();
            sys["role"] = "system";
            sys["content"] = system_prompt;
        }

        // Copy messages, converting Anthropic format to OpenAI
        for (JsonVariant v : messages) {
            JsonObject src = v.as<JsonObject>();
            const char* role = src["role"] | "";
            JsonVariant content = src["content"];

            if (content.is<const char*>()) {
                // Simple text message
                JsonObject m = msgs.add<JsonObject>();
                m["role"] = role;
                m["content"] = content.as<const char*>();
                continue;
            }

            if (!content.is<JsonArray>()) continue;
            JsonArray contentArr = content.as<JsonArray>();

            if (strcmp(role, "assistant") == 0) {
                JsonObject m = msgs.add<JsonObject>();
                m["role"] = "assistant";
                String textBuf;
                JsonArray toolCalls = m["tool_calls"].to<JsonArray>();
                bool hasToolCalls = false;

                for (JsonVariant block : contentArr) {
                    const char* btype = block["type"] | "";
                    if (strcmp(btype, "text") == 0) {
                        textBuf += block["text"].as<String>();
                    } else if (strcmp(btype, "tool_use") == 0) {
                        hasToolCalls = true;
                        JsonObject tc = toolCalls.add<JsonObject>();
                        tc["id"] = block["id"];
                        tc["type"] = "function";
                        JsonObject func = tc["function"].to<JsonObject>();
                        func["name"] = block["name"];
                        // Serialize input object to string
                        String args;
                        serializeJson(block["input"], args);
                        func["arguments"] = args;
                    }
                }
                m["content"] = textBuf.isEmpty() ? "" : textBuf.c_str();
                if (!hasToolCalls) m.remove("tool_calls");
            } else if (strcmp(role, "user") == 0) {
                // tool_result blocks become role=tool messages
                String userText;
                for (JsonVariant block : contentArr) {
                    const char* btype = block["type"] | "";
                    if (strcmp(btype, "tool_result") == 0) {
                        JsonObject tm = msgs.add<JsonObject>();
                        tm["role"] = "tool";
                        tm["tool_call_id"] = block["tool_use_id"];
                        tm["content"] = block["content"] | "";
                    } else if (strcmp(btype, "text") == 0) {
                        userText += block["text"].as<String>();
                    }
                }
                if (!userText.isEmpty()) {
                    JsonObject um = msgs.add<JsonObject>();
                    um["role"] = "user";
                    um["content"] = userText;
                }
            }
        }

        // Convert tools to OpenAI format
        if (tools_json) {
            JsonDocument toolsDoc(&spiram_allocator);
            deserializeJson(toolsDoc, tools_json);
            if (toolsDoc.is<JsonArray>()) {
                JsonArray toolsArr = doc["tools"].to<JsonArray>();
                for (JsonVariant tool : toolsDoc.as<JsonArray>()) {
                    JsonObject wrap = toolsArr.add<JsonObject>();
                    wrap["type"] = "function";
                    JsonObject func = wrap["function"].to<JsonObject>();
                    func["name"] = tool["name"];
                    if (!tool["description"].isNull())
                        func["description"] = tool["description"];
                    if (!tool["input_schema"].isNull())
                        func["parameters"] = tool["input_schema"];
                }
                doc["tool_choice"] = "auto";
            }
        }
    } else {
        // Anthropic format
        doc["max_tokens"] = MIMI_LLM_MAX_TOKENS;
        doc["system"] = system_prompt;

        // Deep-copy messages
        JsonArray msgs = doc["messages"].to<JsonArray>();
        for (JsonVariant v : messages) {
            msgs.add(v);
        }

        // Add tools
        if (tools_json) {
            JsonDocument toolsDoc(&spiram_allocator);
            deserializeJson(toolsDoc, tools_json);
            if (toolsDoc.is<JsonArray>()) {
                doc["tools"] = toolsDoc.as<JsonArray>();
            }
        }
    }

    String output;
    serializeJson(doc, output);
    return output;
}

String MimiLLM::httpDirect(const String& postData) {
    WiFiClientSecure* client = new WiFiClientSecure();
    client->setInsecure(); // Use CA bundle in production
    
    HTTPClient http;
    http.setTimeout(60000);
    http.setReuse(false);

    const char *url = apiUrl();
    MIMI_LOGI(TAG, "access url: %s", url);

    if (!http.begin(*client, url)) {
        MIMI_LOGE(TAG, "HTTP begin failed");
        delete client;
        return "";
    }

    http.addHeader("Content-Type", "application/json");
    if (isOpenAI()) {
        String auth = "Bearer " + String(_apiKey);
        http.addHeader("Authorization", auth);
    } else {
        http.addHeader("x-api-key", _apiKey);
        http.addHeader("anthropic-version", MIMI_LLM_API_VERSION);
    }

    Serial.println(postData);
    
    int httpCode = http.POST(postData);
    String response;
    
    if (httpCode > 0) {
        response = http.getString();
        if (httpCode != 200) {
            MIMI_LOGE(TAG, "API error %d: %.500s", httpCode, response.c_str());
            response = "";
        }
    } else {
        MIMI_LOGE(TAG, "HTTP request failed: [%d]%s", httpCode, http.errorToString(httpCode).c_str());
    }

    http.end();
    delete client;
    return response;
}

String MimiLLM::httpViaProxy(const String& postData) {
    if (!_proxy || !_proxy->isEnabled()) return httpDirect(postData);

    // Connect to proxy
    WiFiClient sock;
    sock.setTimeout(120000);
    if (!sock.connect(_proxy->getHost(), _proxy->getPort())) {
        MIMI_LOGE(TAG, "Cannot connect to proxy");
        return "";
    }

    String proxyType = _proxy->getType();
    
    if (proxyType == "socks5") {
        // SOCKS5 handshake
        uint8_t hs[] = {0x05, 0x01, 0x00};
        sock.write(hs, 3);
        uint8_t resp[2];
        if (sock.readBytes(resp, 2) != 2 || resp[0] != 0x05 || resp[1] != 0x00) {
            MIMI_LOGE(TAG, "SOCKS5 handshake failed");
            sock.stop();
            return "";
        }
        // SOCKS5 connect to API host
        const char* host = apiHost();
        size_t hostLen = strlen(host);
        size_t reqLen = 4 + 1 + hostLen + 2;
        uint8_t* request = (uint8_t*)malloc(reqLen);
        request[0] = 0x05; request[1] = 0x01;
        request[2] = 0x00; request[3] = 0x03;
        request[4] = (uint8_t)hostLen;
        memcpy(request + 5, host, hostLen);
        request[5 + hostLen] = (443 >> 8);
        request[6 + hostLen] = (443 & 0xFF);
        sock.write(request, reqLen);
        free(request);

        uint8_t connResp[10];
        if (sock.readBytes(connResp, 10) < 10 || connResp[1] != 0x00) {
            MIMI_LOGE(TAG, "SOCKS5 connect failed");
            sock.stop();
            return "";
        }
    } else {
        // HTTP CONNECT tunnel
        String connectReq = "CONNECT " + String(apiHost()) + ":443 HTTP/1.1\r\n"
                           "Host: " + String(apiHost()) + ":443\r\n\r\n";
        sock.print(connectReq);
        String statusLine = sock.readStringUntil('\n');
        if (statusLine.indexOf("200") < 0) {
            MIMI_LOGE(TAG, "CONNECT rejected: %s", statusLine.c_str());
            sock.stop();
            return "";
        }
        // Consume headers
        while (true) {
            String line = sock.readStringUntil('\n');
            line.trim();
            if (line.isEmpty()) break;
        }
    }

    // Now do TLS handshake over the tunnel
    WiFiClientSecure tlsClient;
    tlsClient.setInsecure();
    
    // Note: ESP32 Arduino WiFiClientSecure doesn't support taking over existing socket easily.
    // For proxy support, we'll do raw TLS. This is a known limitation.
    // Fallback: just use the established tunnel and send HTTP manually
    // The underlying mbedTLS can work on the existing fd.
    
    // For simplicity in Arduino, we send the raw HTTP request over tunnel.
    // This works for HTTP CONNECT tunnels where TLS is handled transparently.
    sock.stop();
    
    // If proxy is needed but complex TLS-over-tunnel isn't available in Arduino,
    // we use the direct path through the proxy's HTTP support if available.
    MIMI_LOGW(TAG, "Proxy TLS tunnel: falling back to direct connection");
    return httpDirect(postData);
}

bool MimiLLM::chatWithTools(const char* system_prompt, JsonArray messages,
                            const char* tools_json, LlmResponse* resp) {
    memset(resp, 0, sizeof(LlmResponse));

    if (_apiKey[0] == '\0') {
        MIMI_LOGE(TAG, "No API key configured");
        return false;
    }

    String postData = buildRequestBody(system_prompt, messages, tools_json);
    MIMI_LOGI(TAG, "Calling LLM API (provider: %s, model: %s, body: %d bytes)",
              _provider, _model, postData.length());

    String response;
    if (_proxy && _proxy->isEnabled()) {
        response = httpViaProxy(postData);
    } else {
        response = httpDirect(postData);
    }

    if (response.isEmpty()) {
        MIMI_LOGE(TAG, "Empty response from API");
        return false;
    }

    bool ok;
    if (isOpenAI()) {
        ok = parseOpenAIResponse(response, resp);
    } else {
        ok = parseAnthropicResponse(response, resp);
    }

    if (ok) {
        MIMI_LOGI(TAG, "Response: %d bytes text, %d tool calls, stop=%s",
                  (int)resp->text_len, resp->call_count,
                  resp->tool_use ? "tool_use" : "end_turn");
    }
    return ok;
}

bool MimiLLM::parseAnthropicResponse(const String& body, LlmResponse* resp) {
    JsonDocument doc(&spiram_allocator);
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        MIMI_LOGE(TAG, "JSON parse error: %s", err.c_str());
        return false;
    }

    // Check stop_reason
    const char* stop_reason = doc["stop_reason"] | "";
    resp->tool_use = (strcmp(stop_reason, "tool_use") == 0);

    // Parse content blocks
    JsonArray content = doc["content"].as<JsonArray>();
    if (content.isNull()) return false;

    // Accumulate text
    String textBuf;
    for (JsonVariant block : content) {
        const char* btype = block["type"] | "";
        if (strcmp(btype, "text") == 0) {
            textBuf += block["text"].as<String>();
        }
    }

    if (!textBuf.isEmpty()) {
        resp->text = strdup(textBuf.c_str());
        resp->text_len = textBuf.length();
    }

    // Extract tool_use blocks
    for (JsonVariant block : content) {
        const char* btype = block["type"] | "";
        if (strcmp(btype, "tool_use") != 0) continue;
        if (resp->call_count >= MIMI_MAX_TOOL_CALLS) break;

        LlmToolCall* call = &resp->calls[resp->call_count];
        strncpy(call->id, block["id"] | "", sizeof(call->id) - 1);
        strncpy(call->name, block["name"] | "", sizeof(call->name) - 1);

        String inputStr;
        serializeJson(block["input"], inputStr);
        call->input = strdup(inputStr.c_str());
        call->input_len = inputStr.length();

        resp->call_count++;
    }

    return true;
}

bool MimiLLM::parseOpenAIResponse(const String& body, LlmResponse* resp) {
    JsonDocument doc(&spiram_allocator);
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        MIMI_LOGE(TAG, "JSON parse error: %s", err.c_str());
        return false;
    }

    JsonObject choice0 = doc["choices"][0].as<JsonObject>();
    if (choice0.isNull()) return false;

    const char* finishReason = choice0["finish_reason"] | "";
    resp->tool_use = (strcmp(finishReason, "tool_calls") == 0);

    JsonObject message = choice0["message"].as<JsonObject>();
    if (message.isNull()) return false;

    const char* contentStr = message["content"] | "";
    if (contentStr[0]) {
        resp->text = strdup(contentStr);
        resp->text_len = strlen(contentStr);
    }

    JsonArray toolCalls = message["tool_calls"].as<JsonArray>();
    if (!toolCalls.isNull()) {
        for (JsonVariant tc : toolCalls) {
            if (resp->call_count >= MIMI_MAX_TOOL_CALLS) break;
            LlmToolCall* call = &resp->calls[resp->call_count];
            strncpy(call->id, tc["id"] | "", sizeof(call->id) - 1);

            JsonObject func = tc["function"].as<JsonObject>();
            if (!func.isNull()) {
                strncpy(call->name, func["name"] | "", sizeof(call->name) - 1);
                const char* args = func["arguments"] | "{}";
                call->input = strdup(args);
                call->input_len = strlen(args);
            }
            resp->call_count++;
        }
        if (resp->call_count > 0) resp->tool_use = true;
    }

    return true;
}

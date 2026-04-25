#include "mimi_agent.h"
#include "mimi_config.h"
#include <ArduinoJson.h>
#include <esp_heap_caps.h>
#include "arduino_json_psram.h"

static const char* TAG MIMI_TAG_UNUSED = "agent";
#define TOOL_OUTPUT_SIZE  (8 * 1024)

MimiAgent::MimiAgent()
    : _bus(nullptr), _llm(nullptr), _session(nullptr),
      _tools(nullptr), _context(nullptr), _taskHandle(nullptr)
{}

bool MimiAgent::begin(MimiBus* bus, MimiLLM* llm, MimiSession* session,
                       MimiToolRegistry* tools, MimiContext* context) {
    _bus = bus;
    _llm = llm;
    _session = session;
    _tools = tools;
    _context = context;
    MIMI_LOGI(TAG, "Agent loop initialized");
    return true;
}

void MimiAgent::appendTurnContext(char* prompt, size_t size, const MimiMsg* msg) {
    if (!prompt || size == 0 || !msg) return;

    size_t off = strnlen(prompt, size - 1);
    if (off >= size - 1) return;

    snprintf(prompt + off, size - off,
        "\n## Current Turn Context\n"
        "- source_channel: %s\n"
        "- source_chat_id: %s\n"
        "- If using cron_add for Telegram in this turn, set channel='telegram' and chat_id to source_chat_id.\n"
        "- Never use chat_id 'cron' for Telegram messages.\n",
        msg->channel[0] ? msg->channel : "(unknown)",
        msg->chat_id[0] ? msg->chat_id : "(empty)");
}

// Patch cron_add tool input with context from current message
static String patchCronInput(const LlmToolCall* call, const MimiMsg* msg) {
    if (!call || !msg || strcmp(call->name, "cron_add") != 0) return "";

    JsonDocument doc(&spiram_allocator);
    if (deserializeJson(doc, call->input ? call->input : "{}")) {
        doc.clear();
    }

    bool changed = false;
    const char* channel = doc["channel"] | (const char*)nullptr;

    if ((!channel || channel[0] == '\0') && msg->channel[0] != '\0') {
        doc["channel"] = msg->channel;
        channel = msg->channel;
        changed = true;
    }

    if (channel && strcmp(channel, MIMI_CHAN_TELEGRAM) == 0 &&
        strcmp(msg->channel, MIMI_CHAN_TELEGRAM) == 0 && msg->chat_id[0] != '\0') {
        const char* chatId = doc["chat_id"] | (const char*)nullptr;
        if (!chatId || chatId[0] == '\0' || strcmp(chatId, "cron") == 0) {
            doc["chat_id"] = msg->chat_id;
            changed = true;
        }
    } else if (channel && strcmp(channel, MIMI_CHAN_FEISHU) == 0 &&
        strcmp(msg->channel, MIMI_CHAN_FEISHU) == 0 && msg->chat_id[0] != '\0') {
        const char* chatId = doc["chat_id"] | (const char*)nullptr;
        if (!chatId || chatId[0] == '\0' || strcmp(chatId, "cron") == 0) {
            doc["chat_id"] = msg->chat_id;
            changed = true;
        }
    }

    if (changed) {
        String result;
        serializeJson(doc, result);
        MIMI_LOGI(TAG, "Patched cron_add target for %s:%s", msg->channel, msg->chat_id);
        return result;
    }
    return "";
}

void MimiAgent::processMessage(MimiMsg* msg, char* systemPrompt, char* toolOutput) {
    MIMI_LOGI(TAG, "Processing message from %s:%s", msg->channel, msg->chat_id);

    // 1. Build system prompt
    _context->buildSystemPrompt(systemPrompt, MIMI_CONTEXT_BUF_SIZE);
    appendTurnContext(systemPrompt, MIMI_CONTEXT_BUF_SIZE, msg);

    // 2. Load session history
    JsonDocument historyDoc(&spiram_allocator);
    JsonArray messages = historyDoc.to<JsonArray>();
    _session->getHistoryJson(msg->chat_id, messages, MIMI_AGENT_MAX_HISTORY);

    // 3. Append current user message
    JsonObject userMsg = messages.add<JsonObject>();
    userMsg["role"] = "user";
    userMsg["content"] = msg->content;

    // 4. ReAct loop
    char* finalText = nullptr;
    int iteration = 0;
    bool sentWorking = false;

    while (iteration < MIMI_AGENT_MAX_TOOL_ITER) {
#if MIMI_AGENT_SEND_WORKING_STATUS
        if (!sentWorking && strcmp(msg->channel, MIMI_CHAN_SYSTEM) != 0) {
            MimiMsg status;
            memset(&status, 0, sizeof(status));
            strncpy(status.channel, msg->channel, sizeof(status.channel) - 1);
            strncpy(status.chat_id, msg->chat_id, sizeof(status.chat_id) - 1);
            status.content = strdup("\xF0\x9F\x90\xB1mimi is working...");
            if (status.content) {
                if (!_bus->pushOutbound(&status)) {
                    free(status.content);
                } else {
                    sentWorking = true;
                }
            }
        }
#endif

        const char* toolsJson = _tools->getToolsJson();

        LlmResponse resp;
        memset(&resp, 0, sizeof(resp));
        bool ok = _llm->chatWithTools(systemPrompt, messages, toolsJson, &resp);

        if (!ok) {
            MIMI_LOGE(TAG, __LINE__, "LLM call failed");
            break;
        }

        if (!resp.tool_use) {
            // Normal completion
            if (resp.text && resp.text_len > 0) {
                finalText = strdup(resp.text);
            }
            resp.free_response();
            break;
        }

        MIMI_LOGI(TAG, "Tool use iteration %d: %d calls", iteration + 1, resp.call_count);

        // Append assistant message with content array
        JsonObject asstMsg = messages.add<JsonObject>();
        asstMsg["role"] = "assistant";
        JsonArray asstContent = asstMsg["content"].to<JsonArray>();

        if (resp.text && resp.text_len > 0) {
            JsonObject textBlock = asstContent.add<JsonObject>();
            textBlock["type"] = "text";
            textBlock["text"] = resp.text;
        }

        for (int i = 0; i < resp.call_count; i++) {
            LlmToolCall* call = &resp.calls[i];
            JsonObject toolBlock = asstContent.add<JsonObject>();
            toolBlock["type"] = "tool_use";
            toolBlock["id"] = call->id;
            toolBlock["name"] = call->name;

            // Parse input as JSON object
            JsonDocument inputDoc(&spiram_allocator);
            if (call->input && deserializeJson(inputDoc, call->input) == DeserializationError::Ok) {
                toolBlock["input"] = inputDoc.as<JsonVariant>();
            } else {
                toolBlock["input"].to<JsonObject>(); // empty object
            }
        }

        // Execute tools and build results
        JsonObject resultMsg = messages.add<JsonObject>();
        resultMsg["role"] = "user";
        JsonArray resultContent = resultMsg["content"].to<JsonArray>();

        for (int i = 0; i < resp.call_count; i++) {
            LlmToolCall* call = &resp.calls[i];
            const char* toolInput = call->input ? call->input : "{}";

            // Patch cron_add
            String patched = patchCronInput(call, msg);
            if (patched.length() > 0) toolInput = patched.c_str();

            // Execute tool
            toolOutput[0] = '\0';
            _tools->execute(call->name, toolInput, toolOutput, TOOL_OUTPUT_SIZE);

            MIMI_LOGI(TAG, "Tool %s result: %d bytes", call->name, (int)strlen(toolOutput));

            JsonObject resBlock = resultContent.add<JsonObject>();
            resBlock["type"] = "tool_result";
            resBlock["tool_use_id"] = call->id;
            resBlock["content"] = toolOutput;
        }

        resp.free_response();
        iteration++;
    }

    // 5. Send response
    if (finalText && finalText[0]) {
        // Save to session
        _session->append(msg->chat_id, "user", msg->content);
        _session->append(msg->chat_id, "assistant", finalText);

        // Push response to outbound
        MimiMsg out;
        memset(&out, 0, sizeof(out));
        strncpy(out.channel, msg->channel, sizeof(out.channel) - 1);
        strncpy(out.chat_id, msg->chat_id, sizeof(out.chat_id) - 1);
        out.content = finalText;
        MIMI_LOGI(TAG, "Queue response to %s:%s (%d bytes)",
                  out.channel, out.chat_id, (int)strlen(finalText));

        if (!_bus->pushOutbound(&out)) {
            MIMI_LOGW(TAG, "Outbound queue full");
            free(finalText);
        }
    } else {
        free(finalText);
        MimiMsg out;
        memset(&out, 0, sizeof(out));
        strncpy(out.channel, msg->channel, sizeof(out.channel) - 1);
        strncpy(out.chat_id, msg->chat_id, sizeof(out.chat_id) - 1);
        out.content = strdup("Sorry, I encountered an error.");
        if (out.content) {
            if (!_bus->pushOutbound(&out)) free(out.content);
        }
    }
}

void MimiAgent::agentTask(void* arg) {
    MimiAgent* self = (MimiAgent*)arg;
    MIMI_LOGI(TAG, "Agent loop started on core %d", xPortGetCoreID());

    MIMI_LOGI(TAG, "PSRAM total size: %d bytes",
                  (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
                  
    // Allocate large buffers from PSRAM
    char* systemPrompt = (char*)MIMI_CALLOC_PSRAM(1, MIMI_CONTEXT_BUF_SIZE);
    char* toolOutput = (char*)MIMI_CALLOC_PSRAM(1, TOOL_OUTPUT_SIZE);

    if (!systemPrompt || !toolOutput) {
        MIMI_LOGE(TAG, __LINE__, "Failed to allocate PSRAM buffers");
        free(systemPrompt);
        free(toolOutput);
        vTaskDelete(NULL);
        return;
    }

    while (true) {
        MimiMsg msg;
        if (!self->_bus->popInbound(&msg, UINT32_MAX)) continue;

        self->processMessage(&msg, systemPrompt, toolOutput);
        free(msg.content);

        MIMI_LOGI(TAG, "Free PSRAM: %d bytes",
                  (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    }
}

bool MimiAgent::start() {
    if (_taskHandle) return true;

    // Try descending stack sizes
    const uint32_t stackCandidates[] = {
        MIMI_AGENT_STACK, 20 * 1024, 16 * 1024, 14 * 1024, 12 * 1024
    };

    for (size_t i = 0; i < sizeof(stackCandidates) / sizeof(stackCandidates[0]); i++) {
        BaseType_t ret = xTaskCreatePinnedToCore(
            agentTask, "agentloop_task", stackCandidates[i],
            this, MIMI_AGENT_PRIO, &_taskHandle, MIMI_AGENT_CORE);

        if (ret == pdPASS) {
            MIMI_LOGI(TAG, "Agent task created with stack=%u", (unsigned)stackCandidates[i]);
            return true;
        }

        MIMI_LOGW(TAG, "Agent create failed (stack=%u), retrying...", (unsigned)stackCandidates[i]);
    }

    return false;
}

void MimiAgent::stop() {
    if (_taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
        MIMI_LOGI(TAG, "Agent loop stopped");
    }
}

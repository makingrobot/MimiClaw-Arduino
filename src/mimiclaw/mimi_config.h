/*
 * MimiClaw - Global Configuration
 */
#ifndef MIMI_CONFIG_H
#define MIMI_CONFIG_H

/* Build-time secrets (highest priority, override NVS) */
#if __has_include("mimi_secrets.h")
#include "mimi_secrets.h"
#endif

// ── WiFi ───────────────────────────────────────────────────────
#ifndef MIMI_WIFI_SSID
#define MIMI_WIFI_SSID       ""
#endif
#ifndef MIMI_WIFI_PASS
#define MIMI_WIFI_PASS       ""
#endif
#ifndef MIMI_WIFI_MAX_RETRY
#define MIMI_WIFI_MAX_RETRY          10
#endif
#ifndef MIMI_WIFI_RETRY_BASE_MS
#define MIMI_WIFI_RETRY_BASE_MS      1000
#endif
#ifndef MIMI_WIFI_RETRY_MAX_MS
#define MIMI_WIFI_RETRY_MAX_MS       30000
#endif

// ── Telegram ───────────────────────────────────────────────────
#ifndef MIMI_TG_TOKEN
#define MIMI_TG_TOKEN               ""
#endif
#ifndef MIMI_TG_POLL_TIMEOUT_S
#define MIMI_TG_POLL_TIMEOUT_S       30
#endif
#ifndef MIMI_TG_MAX_MSG_LEN
#define MIMI_TG_MAX_MSG_LEN          4096
#endif
#ifndef MIMI_TG_POLL_STACK
#define MIMI_TG_POLL_STACK           (8 * 1024)
#endif
#ifndef MIMI_TG_POLL_PRIO
#define MIMI_TG_POLL_PRIO            5
#endif
#ifndef MIMI_TG_POLL_CORE
#define MIMI_TG_POLL_CORE            0
#endif

// ── Feishu ───────────────────────────────────────────────────
#ifndef MIMI_FEISHU_APP_ID
#define MIMI_FEISHU_APP_ID   ""
#endif
#ifndef MIMI_FEISHU_APP_SECRET
#define MIMI_FEISHU_APP_SECRET ""
#endif
#ifndef MIMI_FEISHU_MAX_MSG_LEN
#define MIMI_FEISHU_MAX_MSG_LEN          4096
#endif
#ifndef MIMI_FEISHU_POLL_STACK
#define MIMI_FEISHU_POLL_STACK           (8 * 1024)
#endif
#ifndef MIMI_FEISHU_POLL_PRIO
#define MIMI_FEISHU_POLL_PRIO            5
#endif
#ifndef MIMI_FEISHU_POLL_CORE
#define MIMI_FEISHU_POLL_CORE            0
#endif

// ── Agent ──────────────────────────────────────────────────────
#ifndef MIMI_AGENT_STACK
#define MIMI_AGENT_STACK             (24 * 1024)
#endif
#ifndef MIMI_AGENT_PRIO
#define MIMI_AGENT_PRIO              6
#endif
#ifndef MIMI_AGENT_CORE
#define MIMI_AGENT_CORE              1
#endif
#ifndef MIMI_AGENT_MAX_HISTORY
#define MIMI_AGENT_MAX_HISTORY       20
#endif
#ifndef MIMI_AGENT_MAX_TOOL_ITER
#define MIMI_AGENT_MAX_TOOL_ITER     10
#endif
#ifndef MIMI_MAX_TOOL_CALLS
#define MIMI_MAX_TOOL_CALLS          4
#endif
#ifndef MIMI_AGENT_SEND_WORKING_STATUS
#define MIMI_AGENT_SEND_WORKING_STATUS 1
#endif

// ── Timezone ───────────────────────────────────────────────────
#ifndef MIMI_TIMEZONE
#define MIMI_TIMEZONE                "PST8PDT,M3.2.0,M11.1.0"
#endif

// ── LLM ────────────────────────────────────────────────────────
#ifndef MIMI_LLM_MODEL
#define MIMI_LLM_MODEL       "claude-opus-4-5"
#endif
#ifndef MIMI_LLM_PROVIDER
#define MIMI_LLM_PROVIDER    "anthropic"
#endif
#ifndef MIMI_LLM_MAX_TOKENS
#define MIMI_LLM_MAX_TOKENS          4096
#endif
#ifndef MIMI_LLM_API_URL
#define MIMI_LLM_API_URL           "https://api.anthropic.com/v1/messages"
#endif
#ifndef MIMI_LLM_API_KEY
#define MIMI_LLM_API_KEY          ""
#endif
#ifndef MIMI_OPENAI_MODEL
#define MIMI_OPENAI_MODEL         ""
#endif
#ifndef MIMI_LLM_API_VERSION
#define MIMI_LLM_API_VERSION         "2023-06-01"
#endif
#ifndef MIMI_LLM_STREAM_BUF_SIZE
#define MIMI_LLM_STREAM_BUF_SIZE     (32 * 1024)
#endif

// ── Message Bus ────────────────────────────────────────────────
#ifndef MIMI_BUS_QUEUE_LEN
#define MIMI_BUS_QUEUE_LEN           16
#endif
#ifndef MIMI_OUTBOUND_STACK
#define MIMI_OUTBOUND_STACK          (12 * 1024)
#endif
#ifndef MIMI_OUTBOUND_PRIO
#define MIMI_OUTBOUND_PRIO           5
#endif
#ifndef MIMI_OUTBOUND_CORE
#define MIMI_OUTBOUND_CORE           0
#endif

// ── FILE / Storage ───────────────────────────────────────────
#ifndef MIMI_FILE_BASE
#define MIMI_FILE_BASE             "/spiffs"
#endif
#ifndef MIMI_FILE_CONFIG_DIR
#define MIMI_FILE_CONFIG_DIR       MIMI_FILE_BASE "/config"
#endif
#ifndef MIMI_FILE_MEMORY_DIR
#define MIMI_FILE_MEMORY_DIR       MIMI_FILE_BASE "/memory"
#endif
#ifndef MIMI_FILE_SESSION_DIR
#define MIMI_FILE_SESSION_DIR      MIMI_FILE_BASE "/sessions"
#endif
#ifndef MIMI_MEMORY_FILE
#define MIMI_MEMORY_FILE             MIMI_FILE_MEMORY_DIR "/MEMORY.md"
#endif
#ifndef MIMI_SOUL_FILE
#define MIMI_SOUL_FILE               MIMI_FILE_CONFIG_DIR "/SOUL.md"
#endif
#ifndef MIMI_USER_FILE
#define MIMI_USER_FILE               MIMI_FILE_CONFIG_DIR "/USER.md"
#endif
#ifndef MIMI_CONTEXT_BUF_SIZE
#define MIMI_CONTEXT_BUF_SIZE        (16 * 1024)
#endif
#ifndef MIMI_SESSION_MAX_MSGS
#define MIMI_SESSION_MAX_MSGS        20
#endif

// ── Cron ───────────────────────────────────────────────────────
#ifndef MIMI_CRON_FILE
#define MIMI_CRON_FILE               MIMI_FILE_BASE "/cron.json"
#endif
#ifndef MIMI_CRON_MAX_JOBS
#define MIMI_CRON_MAX_JOBS           16
#endif
#ifndef MIMI_CRON_CHECK_INTERVAL_MS
#define MIMI_CRON_CHECK_INTERVAL_MS  (60 * 1000)
#endif

// ── Heartbeat ──────────────────────────────────────────────────
#ifndef MIMI_HEARTBEAT_FILE
#define MIMI_HEARTBEAT_FILE          MIMI_FILE_BASE "/HEARTBEAT.md"
#endif
#ifndef MIMI_HEARTBEAT_INTERVAL_MS
#define MIMI_HEARTBEAT_INTERVAL_MS   (30 * 60 * 1000)
#endif

// ── Web search ──────────────────────────────────────────────────
#ifndef MIMI_SEARCH_PROVIDER
#define MIMI_SEARCH_PROVIDER        "tavily"  # tavily | brave
#endif
#ifndef MIMI_TAVILY_KEY
#define MIMI_TAVILY_KEY             ""
#endif
#ifndef MIMI_BRAVE_KEY              
#define MIMI_BRAVE_KEY              ""
#endif

// ── Skills ─────────────────────────────────────────────────────
#ifndef MIMI_SKILLS_PREFIX
#define MIMI_SKILLS_PREFIX           MIMI_FILE_BASE "/skills/"
#endif

// ── WebSocket ──────────────────────────────────────────────────
#ifndef MIMI_WS_PORT
#define MIMI_WS_PORT                 18789
#endif
#ifndef MIMI_WS_MAX_CLIENTS
#define MIMI_WS_MAX_CLIENTS          4
#endif

// ── Channel identifiers ────────────────────────────────────────
#define MIMI_CHAN_TELEGRAM   "telegram"
#define MIMI_CHAN_WEBSOCKET  "websocket"
#define MIMI_CHAN_CLI        "cli"
#define MIMI_CHAN_SYSTEM     "system"
#define MIMI_CHAN_FEISHU     "feishu"

// ── Preferences namespaces ─────────────────────────────────────
#define MIMI_PREF_WIFI      "wifi_config"
#define MIMI_PREF_TG        "tg_config"
#define MIMI_PREF_LLM       "llm_config"
#define MIMI_PREF_PROXY     "proxy_config"
#define MIMI_PREF_SEARCH    "search_config"
#define MIMI_PREF_FS        "feishu_config"

#define MIMI_PREF_WIFI_SSID         "ssid"
#define MIMI_PREF_WIFI_PASSWORD     "password"
#define MIMI_PREF_TG_TOKEN          "token"
#define MIMI_PREF_LLM_APIKEY        "api_key"
#define MIMI_PREF_LLM_MODEL         "model"
#define MIMI_PREF_LLM_PROVIDER      "provider"
#define MIMI_PREF_LLM_APIURL        "api_url"
#define MIMI_PREF_PROXY_HOST        "host"
#define MIMI_PREF_PROXY_PORT        "port"
#define MIMI_PREF_PROXY_TYPE        "proxy_type"
#define MIMI_PREF_SEARCH_BRAVEKEY   "brave_key"
#define MIMI_PREF_SEARCH_TAVILYKEY  "tavily_key"
#define MIMI_PREF_SEARCH_PROVIDER   "provider"
#define MIMI_PREF_FS_APPID          "app_id"
#define MIMI_PREF_FS_APPSECRET      "app_secret"

// ── PSRAM allocation helper ────────────────────────────────────
#ifdef BOARD_HAS_PSRAM
#define MIMI_MALLOC_PSRAM(size) heap_caps_malloc(size, MALLOC_CAP_SPIRAM)
#define MIMI_CALLOC_PSRAM(n, size) heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM)
#else
#define MIMI_MALLOC_PSRAM(size) malloc(size)
#define MIMI_CALLOC_PSRAM(n, size) calloc(n, size)
#endif

// Suppress unused TAG when debug logging is disabled
#define MIMI_TAG_UNUSED __attribute__((unused))

#include "src/framework/sys/log.h"

// ── Logging helpers ────────────────────────────────────────────
#if CONFIG_USE_ESP_LOG==1
#define MIMI_LOGI(tag, fmt, ...) log_i("[%s] " fmt, tag, ##__VA_ARGS__)
#define MIMI_LOGW(tag, fmt, ...) log_w("[%s] " fmt, tag, ##__VA_ARGS__)
#define MIMI_LOGE(tag, fmt, ...) log_e("[%s] " fmt, tag, ##__VA_ARGS__)
#define MIMI_LOGD(tag, fmt, ...) log_d("[%s] " fmt, tag, ##__VA_ARGS__)
#else
#define MIMI_LOGI(tag, fmt, ...) Log::Info(tag, fmt, ##__VA_ARGS__)
#define MIMI_LOGW(tag, fmt, ...) Log::Warn(tag,  fmt, ##__VA_ARGS__)
#define MIMI_LOGE(tag, fmt, ...) Log::Error(tag, fmt, ##__VA_ARGS__)
#define MIMI_LOGD(tag, fmt, ...) Log::Debug(tag, fmt, ##__VA_ARGS__)
#endif

#endif // MIMI_CONFIG_H

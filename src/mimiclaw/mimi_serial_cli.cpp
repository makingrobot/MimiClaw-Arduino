#include "mimi_serial_cli.h"

#include "mimi_config.h"
#include "esp_heap_caps.h"

#include <Arduino.h>
#include <Preferences.h>
#include "mimi_application.h"
#include "mimi_bus.h"
#include "src/framework/board/board.h"
#include "src/framework/file/file_system.h"

#define TAG "serial_cli"

static MimiSerialCli *g_serialcli = nullptr;

MimiSerialCli::MimiSerialCli() {
    g_serialcli = this;
    _msg_queue = xQueueCreate(10, sizeof(MimiMsg));
    if (!_msg_queue) {
        MIMI_LOGE(TAG, "Failed to create message queues");
    }
}

/**
 * cmd: set_wifi ssid password
 */
static int cmd_wifi_set(int argc, char **argv)
{
    if (argc < 3) {
        printf("argument wrong.\n");
        return EXIT_FAILURE;
    }

    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->setWiFiCredentials(argv[1], argv[2]);
    printf("WiFi credentials saved. Restart to apply.\n");
    return 0;
}

/**
 * cmd: wifi_status
 */
static int cmd_wifi_status(int argc, char **argv)
{
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    printf("WiFi connected: %s\n", app->isWiFiConnected() ? "yes" : "no");
    printf("IP: %s\n", app->getIP());
    return 0;
}

/**
 * cmd: set_tg_token token
 */
static int cmd_set_tg_token(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return EXIT_FAILURE;
    }
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->setTelegramToken(argv[1]);
    printf("Telegram bot token saved.\n");
    return 0;
}

/**
 * cmd: set_feishu_creds  app_id  app_secret
 */
static int cmd_set_feishu_creds(int argc, char **argv)
{
    if (argc < 3) {
        printf("argument wrong.\n");
        return EXIT_FAILURE;
    }
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->setFeishuCredentials(argv[1], argv[2]);
    printf("Feishu credentials saved.\n");
    return 0;
}

/**
 * cmd: set_llm_apikey  api_key
 */
static int cmd_set_llm_apikey(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return EXIT_FAILURE;
    }
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->setLLMApiKey(argv[1]);
    printf("LLM API key saved.\n");
    return 0;
}

/**
 * cmd: set_llm_model  model
 */
static int cmd_set_llm_model(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return EXIT_FAILURE;
    }
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->setLLMModel(argv[1]);
    printf("LLM Model set.\n");
    return 0;
}

/**
 * cmd: set_llm_provider  provider
 */
static int cmd_set_llm_provider(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return EXIT_FAILURE;
    }
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->setLLMProvider(argv[1]);
    printf("LLM Provider set.\n");
    return 0;
}

/**
 * cmd: memory_read
 */
static int cmd_memory_read(int argc, char **argv)
{
    char *buf = (char *)malloc(4096);
    if (!buf) {
        printf("Out of memory.\n");
        return 1;
    }
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    if (app->memory().readLongTerm(buf, 4096) && buf[0]) {
        printf("=== MEMORY.md ===\n%s\n=================\n", buf);
    } else {
        printf("MEMORY.md is empty or not found.\n");
    }
    free(buf);
    return 0;
}

/**
 * cmd: memory_write  contents
 */
static int cmd_memory_write(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return 1;
    }
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->memory().writeLongTerm(argv[1]);
    printf("MEMORY.md updated.\n");
    return 0;
}

/**
 * cmd: session_list
 */
static int cmd_session_list(int argc, char **argv)
{
    printf("Sessions:\n");
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->session().list();
    return 0;
}

/**
 * cmd: session_clear  chat_id
 */
static int cmd_session_clear(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return 1;
    }
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    if (app->session().clear(argv[1])) {
        printf("Session cleared.\n");
    } else {
        printf("Session not found.\n");
    }
    return 0;
}

/**
 * cmd: heap_info
 */
static int cmd_heap_info(int argc, char **argv)
{
    printf("Internal free: %d bytes\n",
           (int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("PSRAM free:    %d bytes\n",
           (int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    printf("Total free:    %d bytes\n",
           (int)esp_get_free_heap_size());
    return 0;
}

/**
 * cmd: set_proxy  host  port   (http|sockets)
 */
static int cmd_set_proxy(int argc, char **argv)
{
    if (argc < 4) {
        printf("argument wrong.\n");
        return 1;
    }
    const char *proxy_type = argv[3];
    if (strcmp(proxy_type, "http") != 0 && strcmp(proxy_type, "socks5") != 0) {
        printf("Invalid proxy type: %s. Use http or socks5.\n", proxy_type);
        return 1;
    }

    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->setProxy(argv[1], String(argv[2]).toInt(), proxy_type);
    printf("Proxy set. Restart to apply.\n");
    return 0;
}

/**
 * cmd: clear_proxy
 */
static int cmd_clear_proxy(int argc, char **argv)
{
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->clearProxy();
    printf("Proxy cleared. Restart to apply.\n");
    return 0;
}

/**
 * cmd: set_brave_key  key
 */
static int cmd_set_brave_key(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return 1;
    }
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->setBraveKey(argv[1]);
    printf("Brave key saved.\n");
    return 0;
}

/**
 * cmd: set_tavily_key  key
 */
static int cmd_set_tavily_key(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return 1;
    }
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->setTavilyKey(argv[1]);
    printf("Tavily key saved.\n");
    return 0;
}

/**
 * cmd: set_search_provider  provider
 */
static int cmd_set_search_provider(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return 1;
    }
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    app->setSearchProvider(argv[1]);
    printf("Search Provider saved.\n");
    return 0;
}

/**
 * cmd: skill_list
 */
static int cmd_skill_list(int argc, char **argv)
{
    char *buf = (char *)malloc(4096);
    if (!buf) {
        printf("Out of memory.\n");
        return 1;
    }

    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    size_t n = app->skills().buildSummary(buf, 4096);
    if (n == 0) {
        printf("No skills found under " MIMI_SKILLS_PREFIX ".\n");
    } else {
        printf("=== Skills ===\n%s", buf);
    }
    free(buf);
    return 0;
}

static bool _has_md_suffix(const char *name)
{
    size_t len = strlen(name);
    return (len >= 3) && strcmp(name + len - 3, ".md") == 0;
}

static bool _build_skill_path(const char *name, char *out, size_t out_size)
{
    if (!name || !name[0]) return false;
    if (strstr(name, "..") != NULL) return false;
    if (strchr(name, '/') != NULL || strchr(name, '\\') != NULL) return false;

    if (_has_md_suffix(name)) {
        snprintf(out, out_size, MIMI_SKILLS_PREFIX "%s", name);
    } else {
        snprintf(out, out_size, MIMI_SKILLS_PREFIX "%s.md", name);
    }
    return true;
}

/**
 * cmd: skill_show  name
 */
static int cmd_skill_show(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return 1;
    }

    char path[128];
    if (!_build_skill_path(argv[1], path, sizeof(path))) {
        printf("Invalid skill name.\n");
        return 1;
    }

    FileSystem *fsys = Board::GetInstance().GetFileSystem();
    File f = fsys->OpenFile(path, "r");
    if (!f) {
        printf("Skill not found: %s\n", path);
        return 1;
    }

    printf("=== %s ===\n", path);
    char line[256];
    int len = 0;
    while ((len = f.readBytesUntil('\n', line, sizeof(line))) > 0) {
        line[len] = '\0';
        printf(line);
        printf("\n");
    }
    f.close();
    printf("\n============\n");
    return 0;
}

static bool _contains_nocase(const char *text, const char *keyword)
{
    if (!text || !keyword || !keyword[0]) return false;

    size_t key_len = strlen(keyword);
    for (const char *p = text; *p; p++) {
        size_t i = 0;
        while (i < key_len && p[i] &&
               tolower((unsigned char)p[i]) == tolower((unsigned char)keyword[i])) {
            i++;
        }
        if (i == key_len) return true;
    }
    return false;
}

/**
 * cmd: skill_search keywords
 */
static int cmd_skill_search(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return 1;
    }

    const char *keyword = argv[1];

    FileSystem *fsys = Board::GetInstance().GetFileSystem();
    if (!fsys->ExistsFile(MIMI_SPIFFS_BASE)) {
        printf("Cannot open " MIMI_SPIFFS_BASE ".\n");
        return 1;
    }

    const char *prefix = "skills/";
    const size_t prefix_len = strlen(prefix);
    int matches = 0;

    File dir = fsys->OpenFile(MIMI_SPIFFS_BASE);
    File f;
    while (f = dir.openNextFile()) {
        const char *name = f.name();
        size_t name_len = strlen(name);

        if (strncmp(name, prefix, prefix_len) != 0) continue;
        if (name_len < prefix_len + 4) continue;
        if (strcmp(name + name_len - 3, ".md") != 0) continue;

        char full_path[296];
        snprintf(full_path, sizeof(full_path), MIMI_SPIFFS_BASE "/%s", name);

        bool file_matched = _contains_nocase(name, keyword);
        int matched_line = 0;

        File skill_f = fsys->OpenFile(full_path, "r");
        if (!skill_f) continue;

        char line[256];
        int line_no = 0;
        int len = 0;
        while (!file_matched && (len = skill_f.readBytesUntil('\n', line, sizeof(line))) > 0) {
            line_no++;
            line[len] = '\0';
            if (_contains_nocase(line, keyword)) {
                file_matched = true;
                matched_line = line_no;
            }
        }
        skill_f.close();

        if (file_matched) {
            matches++;
            if (matched_line > 0) {
                printf("- %s (matched at line %d)\n", full_path, matched_line);
            } else {
                printf("- %s (matched in filename)\n", full_path);
            }
        }
    }

    dir.close();
    if (matches == 0) {
        printf("No skills matched keyword: %s\n", keyword);
    } else {
        printf("Total matches: %d\n", matches);
    }
    return 0;
}

static void _print_config(const char *label, const char *ns, const char *key, bool mask)
{
    String source = "not set";
    String value = "(empty)";

    Preferences prefs;
    if (prefs.begin(ns, true)) {
        if (prefs.isKey(key)) {
            value = prefs.getString(key, "");
            source = "NVS";
        }
        prefs.end();
    }

    if (mask && value.length() > 6 && strcmp(value.c_str(), "(empty)") != 0) {
        /* NVS takes highest priority */
        printf("  %-14s: %.4s****  [%s]\n", label, value.c_str(), source.c_str());
    } else {
        printf("  %-14s: %s  [%s]\n", label, value.c_str(), source.c_str());
    }
}

static void _print_config_u16(const char *label, const char *ns, const char *key)
{
    String source = "not set";
    String value = "(empty)";

    Preferences prefs;
    if (prefs.begin(ns, true)) {
        if (prefs.isKey(key)) {
            uint16_t nvs_val = prefs.getUShort(key, 0);
            value = String(nvs_val);
            source = "NVS";
        }
        prefs.end();
    }

    printf("  %-14s: %s  [%s]\n", label, value.c_str(), source.c_str());
}

/**
 * cmd: config_show
 */
static int cmd_config_show(int argc, char **argv)
{
    printf("=== Current Configuration ===\n");
    _print_config("WiFi SSID",          MIMI_PREF_WIFI,     MIMI_PREF_WIFI_SSID,        false);
    _print_config("WiFi Pass",          MIMI_PREF_WIFI,     MIMI_PREF_WIFI_PASSWORD,    true);
    _print_config("TG Token",           MIMI_PREF_TG,       MIMI_PREF_TG_TOKEN,         true);
    _print_config("FS AppId",           MIMI_PREF_FS,       MIMI_PREF_FS_APPID,         false);
    _print_config("FS AppSecret",       MIMI_PREF_FS,       MIMI_PREF_FS_APPSECRET,     true);
    _print_config("LLM API Key",        MIMI_PREF_LLM,      MIMI_PREF_LLM_APIKEY,       true);
    _print_config("LLM API Url",        MIMI_PREF_LLM,      MIMI_PREF_LLM_APIURL,       false);
    _print_config("LLM Model",          MIMI_PREF_LLM,      MIMI_PREF_LLM_MODEL,        false);
    _print_config("LLM Provider",       MIMI_PREF_LLM,      MIMI_PREF_LLM_PROVIDER,     false);
    _print_config("Proxy Host",         MIMI_PREF_PROXY,    MIMI_PREF_PROXY_HOST,       false);
    _print_config_u16("Proxy Port",     MIMI_PREF_PROXY,    MIMI_PREF_PROXY_PORT            );
    _print_config("Search Provider",    MIMI_PREF_SEARCH,   MIMI_PREF_SEARCH_PROVIDER,    false);
    _print_config("Brave Key",          MIMI_PREF_SEARCH,   MIMI_PREF_SEARCH_BRAVEKEY,    true);
    _print_config("Tavily Key",         MIMI_PREF_SEARCH,   MIMI_PREF_SEARCH_TAVILYKEY,    true);
    printf("=============================\n");
    return 0;
}

/**
 * cmd: config_reset
 */
static int cmd_config_reset(int argc, char **argv)
{
    const char *namespaces[] = {
        MIMI_PREF_WIFI,MIMI_PREF_TG,MIMI_PREF_LLM,MIMI_PREF_PROXY,MIMI_PREF_SEARCH,MIMI_PREF_FS
    };
    for (int i = 0; i < sizeof(namespaces); i++) {
        Preferences prefs;
        if (prefs.begin(namespaces[i], false)) {
            prefs.clear();
            prefs.end();
        }
    }
    printf("All NVS config cleared. Build-time defaults will be used on restart.\n");
    return 0;
}

/**
 * cmd: heartbeat_trigger
 */
static int cmd_heartbeat_trigger(int argc, char **argv)
{
    printf("Checking HEARTBEAT.md...\n");
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    if (app->heartbeatTrigger()) {
        printf("Heartbeat: agent prompted with pending tasks.\n");
    } else {
        printf("Heartbeat: no actionable tasks found.\n");
    }
    return 0;
}

/**
 * cmd: cron_start
 */
static int cmd_cron_start(int argc, char **argv)
{
    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    bool success = app->cron().start();
    if (success) {
        printf("Cron service started.\n");
        return 0;
    }

    printf("Failed to start cron service");
    return 1;
}

/**
 * cmd: tool_exec  name  [json]
 */
static int cmd_tool_exec(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: tool_exec <name> [json]\n");
        return 1;
    }

    const char *tool_name = argv[1];
    const char *input_json = (argc >= 3) ? argv[2] : "{}";

    char *output = (char *)calloc(1, 4096);
    if (!output) {
        printf("Out of memory.\n");
        return 1;
    }

    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    bool success = app->tools().execute(tool_name, input_json, output, 4096);
    printf("tool_exec status: %d\n", success ? 0 : 1);
    printf("%s\n", output[0] ? output : "(empty)");
    free(output);
    return success ? 0 : 1;
}

/**
 * cmd: web_search  word [word2 [word3]]
 */
static int cmd_web_search(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return 1;
    }

    String query;
    for (int i=1; i<argc; i++) {
        query += (String(argv[i]) + " ");
    }

    char input_json[640];
    int n = snprintf(input_json, sizeof(input_json), "{\"query\":\"%s\"}", query);
    if (n <= 0 || n >= (int)sizeof(input_json)) {
        printf("Query too long.\n");
        return 1;
    }

    char *output = (char *)calloc(1, 4096);
    if (!output) {
        printf("Out of memory.\n");
        return 1;
    }

    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    bool ret = app->searchWeb(query.c_str(), output, 4096);
   
    printf("web_search status: %d\n", ret ? 0 : 1);
    printf("%s\n", output[0] ? output : "(empty)");
    free(output);
    return ret ? 0 : 1;
}

/**
 * cmd: talk  word [word2 [word3]]
 */
static int cmd_talk(int argc, char **argv)
{
    if (argc < 2) {
        printf("argument wrong.\n");
        return 1;
    }

    String text;
    for (int i=1; i<argc; i++) {
        text += (String(argv[i]) + " ");
    }

    MimiApplication *app = (MimiApplication *)(&Application::GetInstance());
    MimiMsg msg;
    memset(&msg, 0, sizeof(msg));
    strncpy(msg.channel, MIMI_CHAN_CLI, sizeof(msg.channel) - 1);
    strncpy(msg.chat_id, "cli", sizeof(msg.chat_id) - 1);
    msg.content = strdup(text.c_str());
    if (!msg.content) {
        printf("Out of memory.\n");
        return 1;
    }

    if (!app->pushMessage(&msg)) {
        MIMI_LOGW(TAG, "Inbound queue full, dropping message");
        free(msg.content);
        return 1;
    }

    return 0;
}

/**
 * cmd: print_message
 */
static int cmd_print_message(int argc, char **argv)
{
    MimiMsg msg;
    if (xQueueReceive(g_serialcli->msg_queue(), &msg, 1000) == pdTRUE) { //阻塞
        printf("%s\n", msg.content);
        free(msg.content);
        return 0;
    }

    return 1;
}

bool MimiSerialCli::start() {
    //console prompt. By default it is "ESP32>"
    _console.setPrompt("mimi> ");

#if CONFIG_USE_UART1==1
    _console.begin(115200, CONFIG_UART1_RX, CONFIG_UART1_TX, UART_NUM_1);
    MIMI_LOGI(TAG, "MimiClaw console on UART1(rx=GPIO%d,tx=GPIO%d), BAUD=115200", CONFIG_UART1_RX, CONFIG_UART1_TX);
#else
    _console.begin(115200);
    MIMI_LOG(TAG, "MimiClaw Console on UART0, BAUD=115200");
#endif

    //Register builtin commands like 'reboot', 'sysinfo', or 'meminfo'
    //_console.registerSystemCommands();

    /* set_wifi */
    _console.registerCommand(ESP32Console::ConsoleCommand("set_wifi", 
        &cmd_wifi_set, "Set WiFi SSID and password", "WiFi SSID, password"));

    /* wifi_status */
    _console.registerCommand(ESP32Console::ConsoleCommand("wifi_status", 
        &cmd_wifi_status, "Show WiFi connection status"));

    /* set_tg_token */
    _console.registerCommand(ESP32Console::ConsoleCommand("set_tg_token", 
        &cmd_set_tg_token, "Set Telegram bot token", "Telegram bot token"));

    /* set_feishu_creds */
    _console.registerCommand(ESP32Console::ConsoleCommand("set_feishu_creds", 
        &cmd_set_feishu_creds, "Set Feishu app credentials (app_id app_secret)", "Feishu bot AppID, AppSecret"));

    /* set_llm_apikey */
    _console.registerCommand(ESP32Console::ConsoleCommand("set_llm_apikey", 
        &cmd_set_llm_apikey, "Set LLM API key", "LLM API key"));

    /* set_llm_model */
    _console.registerCommand(ESP32Console::ConsoleCommand("set_llm_model", 
        &cmd_set_llm_model, "Set LLM model", "Model identifier"));

    /* set_model_provider */
    _console.registerCommand(ESP32Console::ConsoleCommand("set_llm_provider", 
        &cmd_set_llm_provider, "Set LLM provider)", "Model provider (anthropic|openai)"));

    /* skill_list */
    _console.registerCommand(ESP32Console::ConsoleCommand("skill_list", 
        &cmd_skill_list, "List installed skills from MIMI_SKILLS_PREFIX"));

    /* skill_show */
    _console.registerCommand(ESP32Console::ConsoleCommand("skill_show", 
        &cmd_skill_show, "Print full content of one skill file", "Skill name (e.g. weather or weather.md)"));

    /* skill_search */
    _console.registerCommand(ESP32Console::ConsoleCommand("skill_search", 
        &cmd_skill_search, "Search skill files by keyword (filename + content)", "Keyword to search in skills"));

    /* memory_read */
    _console.registerCommand(ESP32Console::ConsoleCommand("memory_read", 
        &cmd_memory_read, "Read MEMORY.md"));

    /* memory_write */
    _console.registerCommand(ESP32Console::ConsoleCommand("memory_write", 
        &cmd_memory_write, "Write to MEMORY.md", "Content to write"));

    /* session_list */
    _console.registerCommand(ESP32Console::ConsoleCommand("session_list", 
        &cmd_session_list, "List all sessions"));

    /* session_clear */
    _console.registerCommand(ESP32Console::ConsoleCommand("session_clear", 
        &cmd_session_clear, "Clear a session", "Chat ID to clear"));

    /* heap_info */
    _console.registerCommand(ESP32Console::ConsoleCommand("heap_info", 
        &cmd_heap_info, "Show heap memory usage"));

    /* set_brave_key */
    _console.registerCommand(ESP32Console::ConsoleCommand("set_brave_key", 
        &cmd_set_brave_key, "Set brave key for web_search tool", "Brave key"));

    /* set_tavily_key */
    _console.registerCommand(ESP32Console::ConsoleCommand("set_tavily_key", 
        &cmd_set_tavily_key, "Set tavily key for web_search tool", "Tavily key"));

    /* set_search_provider */
    _console.registerCommand(ESP32Console::ConsoleCommand("set_search_provider", 
        &cmd_set_search_provider, "Set Search provider for web_search tool", "Search provider (Brave|Tavily)"));

    /* set_proxy */
    _console.registerCommand(ESP32Console::ConsoleCommand("set_proxy", 
        &cmd_set_proxy, "Set proxy (e.g. set_proxy 192.168.1.83 7897 [http|socks5])", "Proxy host/IP, port, type(http|socket5)"));

    /* clear_proxy */
    _console.registerCommand(ESP32Console::ConsoleCommand("clear_proxy", 
        &cmd_clear_proxy, "Remove proxy configuration"));

    /* config_show */
    _console.registerCommand(ESP32Console::ConsoleCommand("config_show", 
        &cmd_config_show, "Show current configuration (NVS)"));

    /* config_reset */
    _console.registerCommand(ESP32Console::ConsoleCommand("config_reset", 
        &cmd_config_reset, "Clear all NVS overrides, revert to build-time defaults"));

    /* heartbeat_trigger */
    _console.registerCommand(ESP32Console::ConsoleCommand("heartbeat_trigger", 
        &cmd_heartbeat_trigger, "Manually trigger a heartbeat check"));

    /* cron_start */
    _console.registerCommand(ESP32Console::ConsoleCommand("cron_start", 
        &cmd_cron_start, "Start cron scheduler timer now"));

    /* tool_exec */
    _console.registerCommand(ESP32Console::ConsoleCommand("tool_exec", 
        &cmd_tool_exec, "Execute a registered tool: tool_exec <name> '{...json...}'"));

    /* web_search */
    _console.registerCommand(ESP32Console::ConsoleCommand("web_search", 
        &cmd_web_search, "Run web search tool directly (e.g. web_search \"latest esp-idf\")", "Search query"));

    /* talk */
    _console.registerCommand(ESP32Console::ConsoleCommand("talk", 
        &cmd_talk, "Talk with mimiclaw"));

    /* restart */
    _console.registerCommand(ESP32Console::ConsoleCommand("restart", 
        [](int argc, char **argv) -> int {
            printf("Restarting...\n");
            Application::GetInstance().Reboot();
            return EXIT_SUCCESS;
        }, "Restart the device"));

    /* print_message */
    _console.registerCommand(ESP32Console::ConsoleCommand("print_message", 
        &cmd_print_message, "Print message."));
        
    MIMI_LOGI(TAG, "Serial CLI started");
    return true;
}

bool MimiSerialCli::sendMessage(const char* chat_id, const char* text) {
    printf("%s\n", text);

    MimiMsg msg;
    memset(&msg, 0, sizeof(msg));
    strncpy(msg.chat_id, chat_id, sizeof(msg.chat_id) - 1);
    msg.content = strdup(text);

    if (xQueueSend(_msg_queue, &msg, pdMS_TO_TICKS(1000)) != pdTRUE) {
        MIMI_LOGW(TAG, "queue full, dropping message");
        return false;
    }

    int ret = 0;
    esp_console_run("print_message", &ret);
    return true;
}

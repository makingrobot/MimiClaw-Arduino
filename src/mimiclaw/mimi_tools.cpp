#include "mimi_tools.h"
#include "mimi_config.h"
#include "mimi_cron.h"
#include <ArduinoJson.h>
#include <time.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "arduino_json_psram.h"
#include "src/framework/board/board.h"
#include "mimi_application.h"

#define TAG  "tools"

// ── Tool Registry Implementation ───────────────────────────────

MimiToolRegistry::MimiToolRegistry() : _toolCount(0) {}

bool MimiToolRegistry::begin(FileSystem *file_system, MimiProxy* proxy) {
    _file_system = file_system;
    _toolCount = 0;
    registerBuiltinTools();
    MIMI_LOGI(TAG, "Tool registry initialized (%d tools)", _toolCount);
    return true;
}

void MimiToolRegistry::registerTool(const MimiTool* tool) {
    if (_toolCount >= MAX_TOOLS) {
        MIMI_LOGE(TAG, __LINE__, "Tool registry full");
        return;
    }
    _tools[_toolCount++] = *tool;
    MIMI_LOGI(TAG, "Registered tool: [%d]%s", _toolCount-1, tool->name);
}

void MimiToolRegistry::buildToolsJson() {
    JsonDocument doc(&spiram_allocator);
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < _toolCount; i++) {
        JsonObject tool = arr.add<JsonObject>();
        tool["name"] = _tools[i].name;
        tool["description"] = _tools[i].description;
        
        JsonDocument schemaDoc(&spiram_allocator);
        deserializeJson(schemaDoc, _tools[i].input_schema_json);
        tool["input_schema"] = schemaDoc.as<JsonObject>();
    }

    _toolsJson = "";
    serializeJson(arr, _toolsJson);
    MIMI_LOGI(TAG, "Tools JSON built (%d tools)", _toolCount);
}

const char* MimiToolRegistry::getToolsJson() {
    if (_toolsJson.isEmpty()) {
        buildToolsJson();
    }
    return _toolsJson.c_str();
}

bool MimiToolRegistry::execute(const char* name, const char* input_json, char* output, size_t output_size) {
    for (int i = 0; i < _toolCount; i++) {
        if (strcmp(_tools[i].name, name) == 0) {
            MIMI_LOGI(TAG, "Executing tool: %s", name);
            return _tools[i].execute(input_json, output, output_size);
        }
    }
    MIMI_LOGW(TAG, "Unknown tool: %s", name);
    snprintf(output, output_size, "Error: unknown tool '%s'", name);
    return false;
}

// ══════════════════════════════════════════════════════════════════
// Built-in Tool Implementations
// ══════════════════════════════════════════════════════════════════

static const char* MONTHS[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};

static bool parseAndSetTime(const char* dateStr, char* out, size_t outSize) {
    int day, year, hour, min, sec;
    char monStr[4] = {0};

    if (sscanf(dateStr, "%*[^,], %d %3s %d %d:%d:%d",
               &day, monStr, &year, &hour, &min, &sec) != 6)
        return false;

    int mon = -1;
    for (int i = 0; i < 12; i++) {
        if (strcmp(monStr, MONTHS[i]) == 0) { mon = i; break; }
    }
    if (mon < 0) return false;

    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_sec = sec; tm.tm_min = min; tm.tm_hour = hour;
    tm.tm_mday = day; tm.tm_mon = mon; tm.tm_year = year - 1900;

    setenv("TZ", "UTC0", 1);
    tzset();
    time_t t = mktime(&tm);

    setenv("TZ", MIMI_TIMEZONE, 1);
    tzset();

    if (t < 0) return false;

    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&tv, NULL);

    struct tm local;
    localtime_r(&t, &local);
    strftime(out, outSize, "%Y-%m-%d %H:%M:%S %Z (%A)", &local);
    return true;
}

bool tool_get_time_execute(const char* input_json, char* output, size_t output_size) {
    MIMI_LOGI(TAG, "Fetching current time...");

    WiFiClientSecure* client = new WiFiClientSecure();
    client->setInsecure();

    HTTPClient http;
    http.setTimeout(10000);
    
    if (!http.begin(*client, "https://api.telegram.org/")) {
        delete client;
        snprintf(output, output_size, "Error: failed to connect");
        return false;
    }

    // Collect headers
    const char* headerKeys[] = {"Date"};
    http.collectHeaders(headerKeys, 1);
    
    int httpCode = http.sendRequest("HEAD");
    
    if (httpCode <= 0) {
        snprintf(output, output_size, "Error: HTTP request failed");
        http.end();
        delete client;
        return false;
    }

    String dateVal = http.header("Date");
    http.end();
    delete client;

    if (dateVal.isEmpty()) {
        snprintf(output, output_size, "Error: No Date header in response");
        return false;
    }

    if (!parseAndSetTime(dateVal.c_str(), output, output_size)) {
        snprintf(output, output_size, "Error: Failed to parse date: %s", dateVal.c_str());
        return false;
    }

    MIMI_LOGI(TAG, "Time: %s", output);
    return true;
}

// ── File Tools ─────────────────────────────────────────────────

static bool validatePath(const char* path) {
    if (!path) return false;
    size_t baseLen = strlen(MIMI_FILE_BASE);
    if (strncmp(path, MIMI_FILE_BASE, baseLen) != 0) return false;
    if (baseLen > 0 && MIMI_FILE_BASE[baseLen - 1] != '/') {
        if (path[baseLen] != '/') return false;
    }
    if (strstr(path, "..") != NULL) return false;
    return true;
}

bool tool_read_file_execute(const char* input_json, char* output, size_t output_size) {
    JsonDocument doc(&spiram_allocator);
    if (deserializeJson(doc, input_json)) {
        snprintf(output, output_size, "Error: invalid JSON input");
        return false;
    }

    const char* path = doc["path"] | "";
    if (!validatePath(path)) {
        snprintf(output, output_size, "Error: path must start with %s/", MIMI_FILE_BASE);
        return false;
    }

    FileSystem *file_system = Board::GetInstance().GetFileSystem();
    File f = file_system->OpenFile(path, FILE_READ);
    if (!f) {
        snprintf(output, output_size, "Error: file not found: %s", path);
        return false;
    }

    size_t maxRead = output_size - 1;
    if (maxRead > 32768) maxRead = 32768;
    size_t n = f.readBytes(output, maxRead);
    output[n] = '\0';
    f.close();

    MIMI_LOGI(TAG, "read_file: %s (%d bytes)", path, (int)n);
    return true;
}

bool tool_write_file_execute(const char* input_json, char* output, size_t output_size) {
    JsonDocument doc(&spiram_allocator);
    if (deserializeJson(doc, input_json)) {
        snprintf(output, output_size, "Error: invalid JSON input");
        return false;
    }

    const char* path = doc["path"] | "";
    const char* content = doc["content"] | (const char*)nullptr;

    if (!validatePath(path)) {
        snprintf(output, output_size, "Error: path must start with %s/", MIMI_FILE_BASE);
        return false;
    }
    if (!content) {
        snprintf(output, output_size, "Error: missing 'content' field");
        return false;
    }

    FileSystem *file_system = Board::GetInstance().GetFileSystem();
    File f = file_system->OpenFile(path, FILE_WRITE);
    if (!f) {
        snprintf(output, output_size, "Error: cannot open file: %s", path);
        return false;
    }

    size_t written = f.print(content);
    f.close();

    snprintf(output, output_size, "OK: wrote %d bytes to %s", (int)written, path);
    MIMI_LOGI(TAG, "write_file: %s (%d bytes)", path, (int)written);
    return true;
}

bool tool_edit_file_execute(const char* input_json, char* output, size_t output_size) {
    JsonDocument doc(&spiram_allocator);
    if (deserializeJson(doc, input_json)) {
        snprintf(output, output_size, "Error: invalid JSON input");
        return false;
    }

    const char* path = doc["path"] | "";
    const char* oldStr = doc["old_string"] | (const char*)nullptr;
    const char* newStr = doc["new_string"] | (const char*)nullptr;

    if (!validatePath(path)) {
        snprintf(output, output_size, "Error: path must start with %s/", MIMI_FILE_BASE);
        return false;
    }
    if (!oldStr || !newStr) {
        snprintf(output, output_size, "Error: missing old_string or new_string");
        return false;
    }

    FileSystem *file_system = Board::GetInstance().GetFileSystem();
    File f = file_system->OpenFile(path, FILE_READ);
    if (!f) {
        snprintf(output, output_size, "Error: file not found: %s", path);
        return false;
    }

    String fileContent = f.readString();
    f.close();

    int pos = fileContent.indexOf(oldStr);
    if (pos < 0) {
        snprintf(output, output_size, "Error: old_string not found in %s", path);
        return false;
    }

    String result = fileContent.substring(0, pos) + String(newStr) + fileContent.substring(pos + strlen(oldStr));

    f = file_system->OpenFile(path, FILE_WRITE);
    if (!f) {
        snprintf(output, output_size, "Error: cannot write file: %s", path);
        return false;
    }
    f.print(result);
    f.close();

    snprintf(output, output_size, "OK: edited %s (replaced %d bytes with %d bytes)",
             path, (int)strlen(oldStr), (int)strlen(newStr));
    MIMI_LOGI(TAG, "edit_file: %s", path);
    return true;
}

bool tool_list_dir_execute(const char* input_json, char* output, size_t output_size) {
    JsonDocument doc(&spiram_allocator);
    deserializeJson(doc, input_json);
    const char* prefix = doc["prefix"] | (const char*)nullptr;

    FileSystem *file_system = Board::GetInstance().GetFileSystem();
    File root = file_system->OpenFile("/");
    if (!root) {
        snprintf(output, output_size, "Error: cannot open file");
        return false;
    }

    size_t off = 0;
    int count = 0;
    File file = root.openNextFile();
    while (file && off < output_size - 1) {
        String fullPath = String(file.name());
        // file paths may or may not include leading /
        if (!fullPath.startsWith("/")) fullPath = "/" + fullPath;
        String filePath = String(MIMI_FILE_BASE) + fullPath;
        
        if (prefix && strncmp(filePath.c_str(), prefix, strlen(prefix)) != 0) {
            file = root.openNextFile();
            continue;
        }

        off += snprintf(output + off, output_size - off, "%s\n", filePath.c_str());
        count++;
        file = root.openNextFile();
    }

    if (count == 0) snprintf(output, output_size, "(no files found)");
    MIMI_LOGI(TAG, "list_dir: %d files", count);
    return true;
}

void MimiToolRegistry::registerBuiltinTools() {
    // get_current_time
    static const MimiTool gt = {
        "get_current_time",
        "Get the current date and time. Also sets the system clock. Call this when you need to know what time or date it is.",
        "{\"type\":\"object\",\"properties\":{},\"required\":[]}",
        tool_get_time_execute
    };
    registerTool(&gt);

    // read_file
    static const MimiTool rf = {
        "read_file",
        "Read a file from FILE storage. Path must start with " MIMI_FILE_BASE "/.",
        "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Absolute path starting with " MIMI_FILE_BASE "/\"}},\"required\":[\"path\"]}",
        tool_read_file_execute
    };
    registerTool(&rf);

    // write_file
    static const MimiTool wf = {
        "write_file",
        "Write or overwrite a file on FILE storage. Path must start with " MIMI_FILE_BASE "/.",
        "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Absolute path starting with " MIMI_FILE_BASE "/\"},\"content\":{\"type\":\"string\",\"description\":\"File content to write\"}},\"required\":[\"path\",\"content\"]}",
        tool_write_file_execute
    };
    registerTool(&wf);

    // edit_file
    static const MimiTool ef = {
        "edit_file",
        "Find and replace text in a file on FILE store. Replaces first occurrence of old_string with new_string.",
        "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\",\"description\":\"Absolute path starting with " MIMI_FILE_BASE "/\"},\"old_string\":{\"type\":\"string\",\"description\":\"Text to find\"},\"new_string\":{\"type\":\"string\",\"description\":\"Replacement text\"}},\"required\":[\"path\",\"old_string\",\"new_string\"]}",
        tool_edit_file_execute
    };
    registerTool(&ef);

    // list_dir
    static const MimiTool ld = {
        "list_dir",
        "List files on FILE storage, optionally filtered by path prefix.",
        "{\"type\":\"object\",\"properties\":{\"prefix\":{\"type\":\"string\",\"description\":\"Optional path prefix filter\"}},\"required\":[]}",
        tool_list_dir_execute
    };
    registerTool(&ld);
}
/*
 * MimiClaw - Tool Registry
 */
#ifndef MIMI_TOOLS_H
#define MIMI_TOOLS_H

#include <Arduino.h>

// Tool execution function type
typedef bool (*MimiToolExecFn)(const char* input_json, char* output, size_t output_size);

struct MimiTool {
    const char* name;
    const char* description;
    const char* input_schema_json;
    MimiToolExecFn execute;
};

class MimiProxy;  // forward

class MimiToolRegistry {
public:
    MimiToolRegistry();
    bool begin(MimiProxy* proxy = nullptr);

    void registerTool(const MimiTool* tool);
    const char* getToolsJson();
    bool execute(const char* name, const char* input_json, char* output, size_t output_size);

    static const int MAX_TOOLS = 16;

private:
    MimiTool _tools[MAX_TOOLS];
    int _toolCount;
    String _toolsJson;

    void buildToolsJson();
    void registerBuiltinTools();
};

// ── Built-in tool execute functions ────────────────────────────
bool tool_web_search_execute(const char* input_json, char* output, size_t output_size);
bool tool_get_time_execute(const char* input_json, char* output, size_t output_size);
bool tool_read_file_execute(const char* input_json, char* output, size_t output_size);
bool tool_write_file_execute(const char* input_json, char* output, size_t output_size);
bool tool_edit_file_execute(const char* input_json, char* output, size_t output_size);
bool tool_list_dir_execute(const char* input_json, char* output, size_t output_size);
bool tool_cron_add_execute(const char* input_json, char* output, size_t output_size);
bool tool_cron_list_execute(const char* input_json, char* output, size_t output_size);
bool tool_cron_remove_execute(const char* input_json, char* output, size_t output_size);

// Tool init functions
void tool_web_search_init();
void tool_web_search_set_key(const char* key);

#endif // MIMI_TOOLS_H

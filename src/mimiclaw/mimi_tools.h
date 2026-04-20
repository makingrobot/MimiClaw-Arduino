/*
 * MimiClaw-Arduino - Tool Registry
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_TOOLS_H
#define MIMI_TOOLS_H

#include <Arduino.h>
#include "src/framework/file/file_system.h"

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
    bool begin(FileSystem *file_system, MimiProxy* proxy = nullptr);

    void registerTool(const MimiTool* tool);
    const char* getToolsJson();
    bool execute(const char* name, const char* input_json, char* output, size_t output_size);

    static const int MAX_TOOLS = 16;

private:
    FileSystem *_file_system;
    MimiTool _tools[MAX_TOOLS];
    int _toolCount;
    String _toolsJson;

    void buildToolsJson();
    void registerBuiltinTools();
};

#endif // MIMI_TOOLS_H

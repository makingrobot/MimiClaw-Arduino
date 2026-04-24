/*
 * MimiClaw-Arduino - Preferences
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_PREFS_H
#define MIMI_PREFS_H

#include <map>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "src/framework/file/file_system.h"

class MimiPrefs {
public:
    MimiPrefs();
    bool begin(FileSystem* file_system);

    bool containsKey(const char *ns, const char *key);
    
    String getString(const char *ns, const char *key, const char* default_value);
    bool putString(const char *ns, const char *key, const char* value);

    uint16_t getUInt(const char *ns, const char *key, uint16_t default_value);
    bool putUInt(const char *ns, const char *key, uint16_t value);

    int64_t getLong64(const char *ns, const char *key, int64_t default_value);
    bool putLong64(const char *ns, const char *key, int64_t value);

    bool clear(const char *ns);
    bool clearAll();
    void update();

private:
    FileSystem *_filesystem;
    JsonDocument _prefsDoc;
    std::map<String, String> _prefsMap;
};

#endif
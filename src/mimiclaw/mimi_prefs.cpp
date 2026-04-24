#include "mimi_prefs.h"
#include "mimi_config.h"

#if MIMI_PREF_USE_NVS==1
#include <Preferences.h>
#endif

#define TAG          "prefs"
#define PREFERENCES_FILE         "/preferences.json"

MimiPrefs::MimiPrefs() {

}

bool MimiPrefs::begin(FileSystem* file_system) {
    _filesystem = file_system;
    
#if MIMI_PREF_USE_NVS==1
    MIMI_LOGI(TAG, "Using NVS to storge config");
#else
    MIMI_LOGI(TAG, "Using file %s to storge config", PREFERENCES_FILE);

    File f = _filesystem->OpenFile(PREFERENCES_FILE);
    if (!f) {
        MIMI_LOGW(TAG, "%s not exists.", PREFERENCES_FILE);
    }

    String str = f.readString();
    deserializeJson(_prefsDoc, str);
#endif

    return true;
}

bool MimiPrefs::containsKey(const char *ns, const char *key) {
#if MIMI_PREF_USE_NVS==1
    Preferences prefs;
    if (!prefs.begin(ns, true)) {
        return false;
    }

    bool contains = prefs.isKey(key);
    prefs.end();
    return contains;
#else
    if (_prefsDoc.containsKey(ns)) {
        JsonObject obj = _prefsDoc[ns];
        return obj.containsKey(key);
    }

    return false;
#endif
}

String MimiPrefs::getString(const char *ns, const char *key, const char *default_value) {
#if MIMI_PREF_USE_NVS==1
    Preferences prefs;
    if (!prefs.begin(ns, true)) {
        return String(default_value);
    }

    String value = prefs.getString(key, default_value);
    prefs.end();
    return value;
#else
    if (_prefsDoc.containsKey(ns)) {
        JsonObject obj = _prefsDoc[ns];
        if (obj.containsKey(key)) {
            return String(obj[key].as<const char *>());
        }
    }

    return String(default_value);
#endif
}

bool MimiPrefs::putString(const char *ns, const char *key, const char *value) {
#if MIMI_PREF_USE_NVS==1
    Preferences prefs;
    if (!prefs.begin(ns, false)) {
        return false;
    }

    size_t len = prefs.putString(key, value);
    prefs.end();
    return len > 0;
#else
    if (!_prefsDoc.containsKey(ns)) {
        _prefsDoc.createNestedObject(ns);
    }
    String map_key = String(ns)+"_"+key;
    _prefsMap[map_key] = String(value); //cached.
    _prefsDoc[ns][key] = _prefsMap[map_key];
#endif
}

uint16_t MimiPrefs::getUInt(const char *ns, const char *key, uint16_t default_value) {
#if MIMI_PREF_USE_NVS==1
    Preferences prefs;
    if (!prefs.begin(ns, true)) {
        return default_value;
    }

    uint16_t value = prefs.getUInt(key, default_value);
    prefs.end();
    return value;
#else
    if (_prefsDoc.containsKey(ns)) {
        JsonObject obj = _prefsDoc[ns];
        if (obj.containsKey(key)) {
            return obj[key];
        }
    }

    return default_value;
#endif
}

bool MimiPrefs::putUInt(const char *ns, const char *key, uint16_t value) {
#if MIMI_PREF_USE_NVS==1
    Preferences prefs;
    if (!prefs.begin(ns, false)) {
        return false;
    }

    size_t len = prefs.putUInt(key, value);
    prefs.end();
    return len > 0;
#else
    if (!_prefsDoc.containsKey(ns)) {
        _prefsDoc.createNestedObject(ns);
    }
    _prefsDoc[ns][key] = value;
#endif
}

int64_t MimiPrefs::getLong64(const char *ns, const char *key, int64_t default_value) {
#if MIMI_PREF_USE_NVS==1
    Preferences prefs;
    if (!prefs.begin(ns, true)) {
        return default_value;
    }

    int64_t value = prefs.getLong64(key, default_value);
    prefs.end();
    return value;
#else
    if (_prefsDoc.containsKey(ns)) {
        JsonObject obj = _prefsDoc[ns];
        if (obj.containsKey(key)) {
            return obj[key];
        }
    }

    return default_value;
#endif
}

bool MimiPrefs::putLong64(const char *ns, const char *key, int64_t value) {
#if MIMI_PREF_USE_NVS==1
    Preferences prefs;
    if (!prefs.begin(ns, false)) {
        return false;
    }

    size_t len = prefs.putLong64(key, value);
    prefs.end();
    return len > 0;
#else
    if (!_prefsDoc.containsKey(ns)) {
        _prefsDoc.createNestedObject(ns);
    }
    _prefsDoc[ns][key] = value;
#endif
}

bool MimiPrefs::clear(const char *ns) {
#if MIMI_PREF_USE_NVS==1
    Preferences prefs;
    if (prefs.begin(ns, false)) {
        prefs.clear();
        prefs.end();
    } else {
        MIMI_LOGW(TAG, "%s ns open failed.", ns);
    }
#else
    if (_prefsDoc.containsKey(ns)) {
        JsonObject obj = _prefsDoc[ns];
        obj.clear();
        update();
    }
#endif
}

bool MimiPrefs::clearAll() {
#if MIMI_PREF_USE_NVS==1
    std::vector<const char *> namespaces = {
        MIMI_PREF_WIFI, MIMI_PREF_TG, MIMI_PREF_LLM, MIMI_PREF_PROXY, MIMI_PREF_SEARCH, MIMI_PREF_FS
    };
    for (const char* ns : namespaces) {
        Preferences prefs;
        if (prefs.begin(ns, false)) {
            prefs.clear();
            prefs.end();
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            MIMI_LOGW(TAG, "%s ns open for write failed.", ns);
        }
    }
#else
    JsonObject obj = _prefsDoc.as<JsonObject>();
    obj.clear();
    update();

    _prefsMap.clear();
#endif
}

void MimiPrefs::update() {
#if MIMI_PREF_USE_NVS!=1
    File f = _filesystem->OpenFile(PREFERENCES_FILE, FILE_WRITE);
    if (!f) {
        MIMI_LOGE(TAG, __LINE__, "%s open exists.", PREFERENCES_FILE);
        return;
    }

    String str;
    serializeJsonPretty(_prefsDoc, str);

    f.println(str);
    f.close();
#endif
}

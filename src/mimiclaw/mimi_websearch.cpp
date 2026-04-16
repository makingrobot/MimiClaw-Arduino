#include "mimi_websearch.h"
#include "mimi_config.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include "arduino_json_psram.h"
#include "mimi_application.h"

#define TAG "websearch"

// Global cron instance pointer for tool callbacks
static MimiWebsearch* g_websearch = nullptr;

MimiWebsearch::MimiWebsearch() {
    g_websearch = this;
}

void MimiWebsearch::init() {
    Preferences prefs;
    prefs.begin(MIMI_PREF_SEARCH, true);
    String key = prefs.getString("api_key", "");
    prefs.end();
    if (!key.isEmpty()) {
        strncpy(_search_key, key.c_str(), sizeof(_search_key) - 1);
    }
}

void MimiWebsearch::setKey(const char* key) {
    strncpy(_search_key, key, sizeof(_search_key) - 1);
    Preferences prefs;
    prefs.begin(MIMI_PREF_SEARCH, false);
    prefs.putString("api_key", key);
    prefs.end();
    MIMI_LOGI(TAG, "Search API key saved");
}

static size_t url_encode(const char* src, char* dst, size_t dst_size) {
    static const char hex[] = "0123456789ABCDEF";
    size_t pos = 0;
    for (; *src && pos < dst_size - 3; src++) {
        unsigned char c = (unsigned char)*src;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            dst[pos++] = c;
        } else if (c == ' ') {
            dst[pos++] = '+';
        } else {
            dst[pos++] = '%';
            dst[pos++] = hex[c >> 4];
            dst[pos++] = hex[c & 0x0F];
        }
    }
    dst[pos] = '\0';
    return pos;
}

bool MimiWebsearch::search(const char* query, char* output, size_t output_size) {
    
    char encoded[256];
    url_encode(query, encoded, sizeof(encoded));

    String url = "https://api.search.brave.com/res/v1/web/search?q=" + String(encoded) + "&count=5";

    WiFiClientSecure* client = new WiFiClientSecure();
    client->setInsecure();

    HTTPClient http;
    http.setTimeout(15000);
    if (!http.begin(*client, url)) {
        delete client;
        snprintf(output, output_size, "Error: HTTP begin failed");
        return false;
    }

    http.addHeader("Accept", "application/json");
    http.addHeader("X-Subscription-Token", _search_key);

    int httpCode = http.GET();
    if (httpCode != 200) {
        snprintf(output, output_size, "Error: Search API returned %d", httpCode);
        http.end();
        delete client;
        return false;
    }

    String body = http.getString();
    http.end();
    delete client;

    // Parse and format results
    JsonDocument respDoc(&spiram_allocator);
    if (deserializeJson(respDoc, body)) {
        snprintf(output, output_size, "Error: Failed to parse search results");
        return false;
    }

    JsonArray results = respDoc["web"]["results"].as<JsonArray>();
    if (results.isNull() || results.size() == 0) {
        snprintf(output, output_size, "No web results found.");
        return true;
    }

    size_t off = 0;
    int idx = 0;
    for (JsonVariant item : results) {
        if (idx >= 5) break;
        off += snprintf(output + off, output_size - off,
            "%d. %s\n   %s\n   %s\n\n",
            idx + 1,
            item["title"] | "(no title)",
            item["url"] | "",
            item["description"] | "");
        if (off >= output_size - 1) break;
        idx++;
    }

    return true;
}

// ── Tool callbacks ──────────────────

bool tool_web_search_execute(const char* input_json, char* output, size_t output_size) {
    JsonDocument doc(&spiram_allocator);
    if (deserializeJson(doc, input_json)) {
        snprintf(output, output_size, "Error: Invalid input JSON");
        return false;
    }

    const char* query = doc["query"] | "";
    if (!query[0]) {
        snprintf(output, output_size, "Error: Missing 'query' field");
        return false;
    }

    MIMI_LOGI(TAG, "Searching: %s", query);

    bool ret = g_websearch->search(query, output, output_size);
    
    return ret;
}
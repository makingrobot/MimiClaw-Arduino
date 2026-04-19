#include "mimi_websearch.h"
#include "mimi_config.h"
#include "mimi_tools.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include "arduino_json_psram.h"
#include "mimi_application.h"

#define TAG "websearch"

#define SEARCH_BUF_SIZE     (16 * 1024)
#define SEARCH_RESULT_COUNT 5

static MimiWebsearch* g_websearch = nullptr;

bool tool_web_search_execute(const char* input_json, char* output, size_t output_size);

MimiWebsearch::MimiWebsearch() {
    g_websearch = this;

    static const MimiTool ws = {
        "web_search",
        "Search the web for current information. Use this when you need up-to-date facts, news, weather, or anything beyond your training data.",
        "{\"type\":\"object\",\"properties\":{\"query\":{\"type\":\"string\",\"description\":\"The search query\"}},\"required\":[\"query\"]}",
        tool_web_search_execute
    };
    _tools.push_back(&ws);
}

void MimiWebsearch::init() {
    Preferences prefs;
    prefs.begin(MIMI_PREF_SEARCH, true);
    String _provider = prefs.getString(MIMI_PREF_SEARCH_PROVIDER, MIMI_SEARCH_PROVIDER);
    String _brave_key = prefs.getString(MIMI_PREF_SEARCH_BRAVEKEY, MIMI_BRAVE_KEY);
    String _tavily_key = prefs.getString(MIMI_PREF_SEARCH_TAVILYKEY, MIMI_TAVILY_KEY);
    prefs.end();
}

void MimiWebsearch::setBraveKey(const char* key) {
    _brave_key = String(key);
    Preferences prefs;
    prefs.begin(MIMI_PREF_SEARCH, false);
    prefs.putString(MIMI_PREF_SEARCH_BRAVEKEY, _brave_key.c_str());
    prefs.end();
    MIMI_LOGI(TAG, "Brave key saved");
}

void MimiWebsearch::setTavilyKey(const char* key) {
    _tavily_key = String(key);
    Preferences prefs;
    prefs.begin(MIMI_PREF_SEARCH, false);
    prefs.putString(MIMI_PREF_SEARCH_TAVILYKEY, _tavily_key.c_str());
    prefs.end();
    MIMI_LOGI(TAG, "Tavily key saved");
}

void MimiWebsearch::setProvider(const char* provider) {
    _provider = String(provider);
    Preferences prefs;
    prefs.begin(MIMI_PREF_SEARCH, false);
    prefs.putString(MIMI_PREF_SEARCH_PROVIDER, _provider.c_str());
    prefs.end();
    MIMI_LOGI(TAG, "Provider saved");
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
    
    if (strcmp(_provider.c_str(), "brave")==0) {
        if (_brave_key.isEmpty()) {
            MIMI_LOGE(TAG, __LINE__, "Brave key not config.");
            return false;
        }

        return braveSearch(query, output, output_size);

    } else if (strcmp(_provider.c_str(), "tavily") == 0) {
        if (_tavily_key.isEmpty()) {
            MIMI_LOGE(TAG, __LINE__, "Tavily key not config.");
            return false;
        }

        return tavilySearch(query, output, output_size);
    }

    MIMI_LOGE(TAG, __LINE__, "Not specified search provider.");
    return false;
}

bool MimiWebsearch::braveSearch(const char* query, char* output, size_t output_size) {
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
    http.addHeader("X-Subscription-Token", _brave_key.c_str());

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

bool MimiWebsearch::tavilySearch(const char* query, char* output, size_t output_size) {
    char encoded[256];
    url_encode(query, encoded, sizeof(encoded));

    String url = "https://api.tavily.com/search";

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
    http.addHeader("Content-Type", "application/json");
    
    char auth[192];
    snprintf(auth, sizeof(auth), "Bearer %s", _tavily_key.c_str());
    http.addHeader("Authorization", auth);

    JsonDocument payloadDoc(&spiram_allocator);
    payloadDoc["query"] = encoded;
    payloadDoc["max_results"] = SEARCH_RESULT_COUNT;
    payloadDoc["include_answer"] = false;
    payloadDoc["search_depth"] = "baisc";

    String payloadStr;
    serializeJson(payloadDoc, payloadStr);

    int httpCode = http.POST(payloadStr);
    if (httpCode != 200) {
        snprintf(output, output_size, "Error: Tavily API returned %d", httpCode);
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
            item["content"] | "");
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

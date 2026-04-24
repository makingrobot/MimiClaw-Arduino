#include "mimi_websearch.h"
#include "mimi_config.h"
#include "mimi_tools.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
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

void MimiWebsearch::init(MimiPrefs* prefs) {
    _prefs = prefs;
    _provider = _prefs->getString(MIMI_PREF_SEARCH, MIMI_PREF_SEARCH_PROVIDER, MIMI_SEARCH_PROVIDER);
    _brave_key = _prefs->getString(MIMI_PREF_SEARCH, MIMI_PREF_SEARCH_BRAVEKEY, MIMI_BRAVE_KEY);
    _tavily_key = _prefs->getString(MIMI_PREF_SEARCH, MIMI_PREF_SEARCH_TAVILYKEY, MIMI_TAVILY_KEY);

    if (_provider.isEmpty()) {
        MIMI_LOGI(TAG, "Websearch provider not set.");
    } else {
        if (strcmp("tavily", _provider.c_str()) == 0) {
            MIMI_LOGI(TAG, _tavily_key.isEmpty() ? "No Tavily key configured." : "Using tavily websearch.");
        } else if (strcmp("brave", _provider.c_str()) == 0) {
            MIMI_LOGI(TAG, _brave_key.isEmpty() ? "No Brave key configured." : "Using brave websearch.");
        }
    }
}

void MimiWebsearch::setBraveKey(const char* key) {
    _brave_key = String(key);
    _prefs->putString(MIMI_PREF_SEARCH, MIMI_PREF_SEARCH_BRAVEKEY, _brave_key.c_str());
    _prefs->update();
    MIMI_LOGI(TAG, "Brave key saved");
}

void MimiWebsearch::setTavilyKey(const char* key) {
    _tavily_key = String(key);
    _prefs->putString(MIMI_PREF_SEARCH, MIMI_PREF_SEARCH_TAVILYKEY, _tavily_key.c_str());
    _prefs->update();
    MIMI_LOGI(TAG, "Tavily key saved");
}

void MimiWebsearch::setProvider(const char* provider) {
    _provider = String(provider);
    _prefs->putString(MIMI_PREF_SEARCH, MIMI_PREF_SEARCH_PROVIDER, _provider.c_str());
    _prefs->update();
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
            MIMI_LOGE(TAG, __LINE__, "No Brave key configured.");
            return false;
        }

        MIMI_LOGI(TAG, "Use Brave search.");
        return braveSearch(query, output, output_size);

    } else if (strcmp(_provider.c_str(), "tavily") == 0) {
        if (_tavily_key.isEmpty()) {
            MIMI_LOGE(TAG, __LINE__, "No Tavily key configured.");
            return false;
        }

        MIMI_LOGI(TAG, "Use Tavily search.");
        return tavilySearch(query, output, output_size);
    }

    MIMI_LOGE(TAG, __LINE__, "Not specified search provider.");
    return false;
}

bool MimiWebsearch::braveSearch(const char* query, char* output, size_t output_size) {
    char encoded[256] = {0};
    url_encode(query, encoded, sizeof(encoded));

    String url = "https://api.search.brave.com/res/v1/web/search?q=" + String(encoded) + "&count=5";

    WiFiClientSecure* client = new WiFiClientSecure();
    client->setInsecure();

    HTTPClient http;
    http.setTimeout(15000);
    if (!http.begin(*client, url)) {
        delete client;
        snprintf(output, output_size, "Error: HTTP begin failed");
        MIMI_LOGE(TAG, __LINE__, "Error: HTTP begin failed");
        return false;
    }

    http.addHeader("Accept", "application/json");
    http.addHeader("X-Subscription-Token", _brave_key);

    int httpCode = http.GET();
    if (httpCode != 200) {
        snprintf(output, output_size, "Error: Search API returned %d", httpCode);
        MIMI_LOGE(TAG, __LINE__, "Error: Search API returned %d", httpCode);
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
        MIMI_LOGE(TAG, __LINE__, "Error: Failed to parse search results");
        return false;
    }

    JsonArray results = respDoc["web"]["results"].as<JsonArray>();
    if (results.isNull() || results.size() == 0) {
        snprintf(output, output_size, "Brave search no results.");
        MIMI_LOGI(TAG, "Brave search no results.");
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

// curl --request POST \
//   --url https://api.tavily.com/search \
//   --header 'Authorization: Bearer <token>' \
//   --header 'Content-Type: application/json' \
//   --data '
// {
//   "query": "who is Leo Messi?",
//   "auto_parameters": false,
//   "topic": "general",
//   "search_depth": "basic",
//   "chunks_per_source": 3,
//   "max_results": 1,
//   "time_range": null,
//   "start_date": "2025-02-09",
//   "end_date": "2025-12-29",
//   "include_answer": false,
//   "include_raw_content": false,
//   "include_images": false,
//   "include_image_descriptions": false,
//   "include_favicon": false,
//   "include_domains": [],
//   "exclude_domains": [],
//   "country": null,
//   "include_usage": false
// }
bool MimiWebsearch::tavilySearch(const char* query, char* output, size_t output_size) {
    char encoded[256] = {0};
    url_encode(query, encoded, sizeof(encoded));

    String url = "https://api.tavily.com/search";

    WiFiClientSecure* client = new WiFiClientSecure();
    client->setInsecure();

    HTTPClient http;
    http.setTimeout(15000);

    if (!http.begin(*client, url)) {
        delete client;
        snprintf(output, output_size, "Error: HTTP begin failed");
        MIMI_LOGE(TAG, __LINE__, output);
        return false;
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + _tavily_key);
    MIMI_LOGD(TAG, __LINE__, "Authorization: Bearer %s", _tavily_key.c_str());

    JsonDocument payloadDoc(&spiram_allocator);
    payloadDoc["query"] = encoded;
    payloadDoc["max_results"] = SEARCH_RESULT_COUNT;
    payloadDoc["search_depth"] = "advanced";

    String payloadStr;
    serializeJson(payloadDoc, payloadStr);

    int httpCode = http.POST(payloadStr);
    if (httpCode != 200) {
        snprintf(output, output_size, "Error: Tavily API returned %d", httpCode);
        MIMI_LOGE(TAG, __LINE__, "Error: Tavily API returned %d", httpCode);
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
        MIMI_LOGE(TAG, __LINE__, "Error: Failed to parse search results");
        return false;
    }

    JsonArray results = respDoc["results"].as<JsonArray>();
    if (results.isNull() || results.size() == 0) {
        snprintf(output, output_size, "Tavily search no results found.");
        MIMI_LOGI(TAG, "Tavily search no results found.");
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
        MIMI_LOGE(TAG, __LINE__, "Error: Invalid input JSON");
        return false;
    }

    const char* query = doc["query"] | "";
    if (!query[0]) {
        snprintf(output, output_size, "Error: Missing 'query' field");
        MIMI_LOGE(TAG, __LINE__, "Error: Missing 'query' field");
        return false;
    }

    MIMI_LOGI(TAG, "Searching: %s", query);

    bool ret = g_websearch->search(query, output, output_size);
    
    return ret;
}

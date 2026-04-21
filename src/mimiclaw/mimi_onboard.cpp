#include "mimi_onboard.h"
#include "mimi_config.h"
#include <esp_mac.h>
#include "WiFi.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "arduino_json_psram.h"
#include <Preferences.h>
#include "onboard_html.h"

#define TAG "onboard"

static const IPAddress ap_ip(192,168,4,1);
static const IPAddress ap_gateway(192,168,4,1);
static const IPAddress ap_subnet(255,255,255,0);

MimiOnboard::MimiOnboard() {

}

void MimiOnboard::webTask(void* arg) {
    MimiOnboard* self = (MimiOnboard*)arg;
    while (true) {
        self->_webserver->handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool MimiOnboard::start(bool admin) {
    if (_webserver) return true;

    MIMI_LOGI(TAG, "========================================");
    MIMI_LOGI(TAG, "  Starting WiFi Configuration Portal");
    MIMI_LOGI(TAG, "========================================");

    // 开始热点
    String ap_ssid = getSsid();
    WiFi.mode(admin ? WIFI_AP_STA : WIFI_AP);
    WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet);
    if (!WiFi.softAP(ap_ssid.c_str())) {
        MIMI_LOGE(TAG, __LINE__, "Soft AP creation failed.");
        return false;
    }
    MIMI_LOGI(TAG, "Wifi AP %s on %s", ap_ssid.c_str(), ap_ip.toString());

    _webserver = new WebServer(80);
    if (!_webserver) {
        MIMI_LOGE(TAG, __LINE__, "Failed to create Webserver");
        return false;
    }

    _webserver->on("/", HTTP_GET, [this](){ handleRoot(); });

    _webserver->on("/scan", HTTP_GET, [this](){ handleScan(); });

    _webserver->on("/config", HTTP_GET, [this](){ handleConfig(); });

    _webserver->on("/save", HTTP_POST, [this]() { handleSave(); }, [this](){ 
            // 当contentType为 applicaton/json时，使用raw获取JSON数据。
            HTTPRaw &raw = _webserver->raw();
            if (raw.status == RAW_START) {
                _saved_data = "";
                MIMI_LOGD(TAG, __LINE__, "Upload: START");
            } else if (raw.status == RAW_WRITE) {
                _saved_data = _saved_data + String(reinterpret_cast<char*>(raw.buf), raw.currentSize);
            } else if (raw.status == RAW_END) {
                MIMI_LOGD(TAG, __LINE__, "Upload: END, Size: %d", raw.totalSize);
            }
        });

    _webserver->begin();

    MIMI_LOGI(TAG, "WebServer started on port 80");

    // Create a task to run the WS loop
    BaseType_t ok = xTaskCreate(webTask, "web_srv", 4096, this, 4, &_taskHandle);
    if (ok != pdPASS) {
        MIMI_LOGE(TAG, __LINE__, "Failed to create Web task");
        return false;
    }

    if (!admin) {
        /* Block forever — onboarding ends with esp_restart() in /save handler */
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(60000));
            MIMI_LOGI(TAG, "Waiting for configure wifi ...");
        }
        return false;
    }

    return true;
}

void MimiOnboard::stop() {
    if (_taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
    if (_webserver) {
        _webserver->stop();
        delete _webserver;
        _webserver = nullptr;
        MIMI_LOGI(TAG, "WebServer stopped");
    }
}

String MimiOnboard::getSsid() {
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP));
    char ssid[32] = { 0 };
    snprintf(ssid, sizeof(ssid)-1, "mimiclaw-%02x%02x", mac[4], mac[5]);
    return String(ssid);
}

void MimiOnboard::handleRoot() {
    _webserver->send(200, "text/html", ONBOARD_HTML);
}

void MimiOnboard::handleScan() {
    // 同步扫描
    int n = WiFi.scanNetworks();
    if (n == WIFI_SCAN_FAILED) {
        MIMI_LOGW(TAG, "Wi-Fi扫描启动失败，请检查硬件或驱动状态");
        _webserver->send(500, "text/plain", "Scan failed.");
        return;
    }

    JsonDocument ssidDoc(&spiram_allocator);
    JsonArray ssidArr = ssidDoc.to<JsonArray>();

    for (int i=0; i<n; ++i) {
        if (WiFi.SSID(i).isEmpty()) continue;  /* skip hidden */

        JsonObject item = ssidArr.add<JsonObject>();
        item["ssid"] = WiFi.SSID(i);  //string
        item["rssi"] = WiFi.RSSI(i);  //num
        item["ch"] = WiFi.channel(i);  //num
        item["auth"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN; //bool
    }

    WiFi.scanDelete();
    
    String ssidStr;
    serializeJson(ssidArr, ssidStr);
    _webserver->send(200, "application/json", ssidStr);
}

static void _addConfigItem(JsonDocument &doc, const char *json_key,
                                      const char *ns, const char *key, const char *build_val)
{
    bool found = false;
    String val;
    Preferences prefs;
    if (prefs.begin(ns, true)) {
        if (prefs.isKey(key)) {
            val = prefs.getString(key);
            found = true;
        }
        prefs.end();
    }

    if (!found && build_val) {
        val = String(build_val);
    }

    doc[json_key] = val;
}

static void _addConfigItemUint(JsonDocument &doc, const char *json_key,
                                      const char *ns, const char *key, const char *build_val)
{
    bool found = false;
    uint16_t val;
    Preferences prefs;
    if (prefs.begin(ns, true)) {
        if (prefs.isKey(key)) {
            val = prefs.getUInt(key);
            found = true;
        }
        prefs.end();
    }

    if (!found && build_val) {
        val = (uint16_t)(String(build_val).toInt());
    }

    doc[json_key] = val;
}

void MimiOnboard::handleConfig() {
    JsonDocument configDoc(&spiram_allocator);

    _addConfigItem(configDoc, "ssid", MIMI_PREF_WIFI, MIMI_PREF_WIFI_SSID, MIMI_WIFI_SSID);
    _addConfigItem(configDoc, "password", MIMI_PREF_WIFI, MIMI_PREF_WIFI_PASS, MIMI_WIFI_PASS);
    _addConfigItem(configDoc, "llm_api_key", MIMI_PREF_LLM, MIMI_PREF_LLM_APIKEY, MIMI_LLM_API_KEY);
    _addConfigItem(configDoc, "llm_model", MIMI_PREF_LLM, MIMI_PREF_LLM_MODEL, MIMI_LLM_MODEL);
    _addConfigItem(configDoc, "llm_provider", MIMI_PREF_LLM, MIMI_PREF_LLM_PROVIDER, MIMI_LLM_PROVIDER);
    _addConfigItem(configDoc, "tg_token", MIMI_PREF_TG, MIMI_PREF_TG_TOKEN, MIMI_TG_TOKEN);
    _addConfigItem(configDoc, "feishu_app_id", MIMI_PREF_FS, MIMI_PREF_FS_APPID, MIMI_FEISHU_APP_ID);
    _addConfigItem(configDoc, "feishu_app_secret", MIMI_PREF_FS, MIMI_PREF_FS_APPSECRET, MIMI_FEISHU_APP_SECRET);
    _addConfigItem(configDoc, "proxy_host", MIMI_PREF_PROXY, MIMI_PREF_PROXY_HOST, MIMI_PROXY_HOST);
    _addConfigItemUint(configDoc, "proxy_port", MIMI_PREF_PROXY, MIMI_PREF_PROXY_PORT, MIMI_PROXY_PORT);
    _addConfigItem(configDoc, "proxy_type", MIMI_PREF_PROXY, MIMI_PREF_PROXY_TYPE, MIMI_PROXY_TYPE);
    _addConfigItem(configDoc, "search_provider", MIMI_PREF_SEARCH, MIMI_PREF_SEARCH_PROVIDER, MIMI_SEARCH_PROVIDER);
    _addConfigItem(configDoc, "tavily_key", MIMI_PREF_SEARCH, MIMI_PREF_SEARCH_TAVILYKEY, MIMI_TAVILY_KEY);
    _addConfigItem(configDoc, "brave_key", MIMI_PREF_SEARCH, MIMI_PREF_SEARCH_BRAVEKEY, MIMI_BRAVE_KEY);

    String configJson;
    serializeJson(configDoc, configJson);
    _webserver->send(200, "application/json", configJson);
}

void MimiOnboard::handleSave() {
    JsonDocument postDoc(&spiram_allocator);
    deserializeJson(postDoc, _saved_data);

    Preferences prefs;
    if (prefs.begin(MIMI_PREF_WIFI, false)) {
        prefs.putString(MIMI_PREF_WIFI_SSID, postDoc["ssid"].as<const char *>());
        prefs.putString(MIMI_PREF_WIFI_PASS, postDoc["password"].as<const char *>());
        prefs.end();
    }

    if (prefs.begin(MIMI_PREF_LLM, false)) {
        prefs.putString(MIMI_PREF_LLM_APIKEY, postDoc["llm_api_key"].as<const char *>());
        prefs.putString(MIMI_PREF_LLM_MODEL, postDoc["llm_api_model"].as<const char *>());
        prefs.putString(MIMI_PREF_LLM_PROVIDER, postDoc["llm_provider"].as<const char *>());
        prefs.end();
    }

    if (prefs.begin(MIMI_PREF_TG, false)) {
        prefs.putString(MIMI_PREF_TG_TOKEN, postDoc["tg_token"].as<const char *>());
        prefs.end();
    }

    if (prefs.begin(MIMI_PREF_FS, false)) {
        prefs.putString(MIMI_PREF_FS_APPID, postDoc["feishu_app_id"].as<const char *>());
        prefs.putString(MIMI_PREF_FS_APPSECRET, postDoc["feishu_app_secret"].as<const char *>());
        prefs.end();
    }

    if (prefs.begin(MIMI_PREF_PROXY, false)) {
        prefs.putString(MIMI_PREF_PROXY_HOST, postDoc["proxy_host"].as<const char *>());
        prefs.putString(MIMI_PREF_PROXY_TYPE, postDoc["proxy_type"].as<const char *>());
        
        String str = postDoc["proxy_port"].as<const char *>();
        prefs.putUInt(MIMI_PREF_PROXY_PORT, str.isEmpty() ? 0 : str.toInt());
        prefs.end();
    }

    if (prefs.begin(MIMI_PREF_SEARCH, false)) {
        prefs.putString(MIMI_PREF_SEARCH_PROVIDER, postDoc["search_provider"].as<const char *>());
        prefs.putString(MIMI_PREF_SEARCH_TAVILYKEY, postDoc["tavily_key"].as<const char *>());
        prefs.putString(MIMI_PREF_SEARCH_BRAVEKEY, postDoc["brave_key"].as<const char *>());
        prefs.end();
    }

    _webserver->send(200, "application/json", "{\"ok\":true}");

    MIMI_LOGI(TAG, "Configuration saved, restarting in 2s...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
}
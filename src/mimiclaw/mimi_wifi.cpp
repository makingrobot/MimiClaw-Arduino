#include "mimi_wifi.h"
#include "mimi_config.h"

static const char* TAG MIMI_TAG_UNUSED = "wifi";

MimiWiFi* MimiWiFi::_instance = nullptr;

MimiWiFi::MimiWiFi() : _connected(false), _retryCount(0) {
    _instance = this;
    _ipStr = "0.0.0.0";
}

bool MimiWiFi::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.onEvent(eventHandler);
    MIMI_LOGI(TAG, "WiFi manager initialized");
    return true;
}

void MimiWiFi::setCredentials(const char* ssid, const char* pass) {
    _ssid = ssid;
    _pass = pass;
    // Also save to Preferences
    _prefs.begin(MIMI_PREF_WIFI, false);
    _prefs.putString(MIMI_PREF_WIFI_SSID, ssid);
    _prefs.putString(MIMI_PREF_WIFI_PASSWORD, pass);
    _prefs.end();
    MIMI_LOGI(TAG, "WiFi credentials saved for SSID: %s", ssid);
}

bool MimiWiFi::start() {
    // Load from Preferences if not set
    if (_ssid.isEmpty()) {
        _prefs.begin(MIMI_PREF_WIFI, true);
        _ssid = _prefs.getString(MIMI_PREF_WIFI_SSID, MIMI_WIFI_SSID);
        _pass = _prefs.getString(MIMI_PREF_WIFI_PASSWORD, MIMI_WIFI_PASS);
        _prefs.end();
    }

    if (_ssid.isEmpty()) {
        MIMI_LOGW(TAG, "No WiFi credentials configured");
        return false;
    }

    MIMI_LOGI(TAG, "Connecting to SSID: %s", _ssid.c_str());
    WiFi.begin(_ssid.c_str(), _pass.c_str());
    return true;
}

bool MimiWiFi::waitConnected(uint32_t timeout_ms) {
    uint32_t start = millis();
    while (!_connected && (millis() - start) < timeout_ms) {
        delay(100);
    }
    return _connected;
}

bool MimiWiFi::isConnected() {
    return WiFi.isConnected();
}

const char* MimiWiFi::getIP() {
    _ipStr = WiFi.localIP().toString();
    return _ipStr.c_str();
}

void MimiWiFi::scanAndPrint() {
    MIMI_LOGI(TAG, "Scanning nearby APs...");
    int n = WiFi.scanNetworks();
    if (n == 0) {
        MIMI_LOGW(TAG, "No APs found");
    } else {
        MIMI_LOGI(TAG, "Found %d APs:", n);
        for (int i = 0; i < n; i++) {
            MIMI_LOGI(TAG, "  [%d] SSID=%s RSSI=%d CH=%d",
                       i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
        }
    }
    WiFi.scanDelete();
}

void MimiWiFi::eventHandler(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (!_instance) return;
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            _instance->_onConnected(WiFi.localIP());
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            _instance->_onDisconnected(info.wifi_sta_disconnected.reason);
            break;
        default:
            break;
    }
}

void MimiWiFi::_onConnected(IPAddress ip) {
    _connected = true;
    _retryCount = 0;
    _ipStr = ip.toString();
    MIMI_LOGI(TAG, "Connected! IP: %s", _ipStr.c_str());
}

void MimiWiFi::_onDisconnected(uint8_t reason) {
    _connected = false;
    MIMI_LOGW(TAG, "Disconnected (reason=%d)", reason);
    if (_retryCount < MIMI_WIFI_MAX_RETRY) {
        uint32_t delay_ms = MIMI_WIFI_RETRY_BASE_MS << _retryCount;
        if (delay_ms > MIMI_WIFI_RETRY_MAX_MS) delay_ms = MIMI_WIFI_RETRY_MAX_MS;
        MIMI_LOGW(TAG, "Retry %d/%d in %u ms", _retryCount + 1, MIMI_WIFI_MAX_RETRY, delay_ms);
        delay(delay_ms);
        WiFi.reconnect();
        _retryCount++;
    } else {
        MIMI_LOGE(TAG, "Failed to connect after %d retries", MIMI_WIFI_MAX_RETRY);
    }
}

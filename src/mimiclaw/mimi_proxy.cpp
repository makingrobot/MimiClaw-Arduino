#include "mimi_proxy.h"
#include "mimi_config.h"

static const char* TAG MIMI_TAG_UNUSED = "proxy";

MimiProxy::MimiProxy() : _port(0), _type("http") {}

bool MimiProxy::begin() {
    // Load from Preferences
    _prefs.begin(MIMI_PREF_PROXY, true);
    _host = _prefs.getString(MIMI_PREF_PROXY_HOST, "");
    _port = _prefs.getUShort(MIMI_PREF_PROXY_PORT, 0);
    _type = _prefs.getString(MIMI_PREF_PROXY_TYPE, "http");
    _prefs.end();

    if (isEnabled()) {
        MIMI_LOGI(TAG, "Proxy configured: %s:%d (%s)", _host.c_str(), _port, _type.c_str());
    } else {
        MIMI_LOGI(TAG, "No proxy configured (direct connection)");
    }
    return true;
}

bool MimiProxy::isEnabled() {
    return !_host.isEmpty() && _port != 0;
}

void MimiProxy::set(const char* host, uint16_t port, const char* type) {
    _host = host;
    _port = port;
    _type = type;

    _prefs.begin(MIMI_PREF_PROXY, false);
    _prefs.putString(MIMI_PREF_PROXY_HOST, host);
    _prefs.putUShort(MIMI_PREF_PROXY_PORT, port);
    _prefs.putString(MIMI_PREF_PROXY_TYPE, type);
    _prefs.end();

    MIMI_LOGI(TAG, "Proxy set to %s:%d (%s)", _host.c_str(), _port, _type.c_str());
}

void MimiProxy::clear() {
    _prefs.begin(MIMI_PREF_PROXY, false);
    _prefs.clear();
    _prefs.end();

    _host = "";
    _port = 0;
    _type = "http";
    MIMI_LOGI(TAG, "Proxy cleared");
}

bool MimiProxy::connectTunnel(WiFiClient& sock, const char* targetHost, int targetPort, int timeout_ms) {
    sock.setTimeout(timeout_ms);
    if (!sock.connect(_host.c_str(), _port)) {
        MIMI_LOGE(TAG, "TCP connect to proxy %s:%d failed", _host.c_str(), _port);
        return false;
    }
    MIMI_LOGI(TAG, "Connected to proxy %s:%d", _host.c_str(), _port);

    // Send CONNECT request
    String req = String("CONNECT ") + targetHost + ":" + String(targetPort) + " HTTP/1.1\r\n"
                 + "Host: " + targetHost + ":" + String(targetPort) + "\r\n\r\n";
    sock.print(req);

    // Read response line
    String statusLine = sock.readStringUntil('\n');
    if (statusLine.indexOf("200") < 0) {
        MIMI_LOGE(TAG, "CONNECT rejected: %s", statusLine.c_str());
        return false;
    }

    // Consume remaining headers
    while (true) {
        String line = sock.readStringUntil('\n');
        line.trim();
        if (line.isEmpty()) break;
    }

    MIMI_LOGI(TAG, "CONNECT tunnel established to %s:%d", targetHost, targetPort);
    return true;
}

bool MimiProxy::socks5Handshake(WiFiClient& sock, const char* targetHost, int targetPort, int timeout_ms) {
    sock.setTimeout(timeout_ms);
    if (!sock.connect(_host.c_str(), _port)) {
        MIMI_LOGE(TAG, "TCP connect to SOCKS5 proxy %s:%d failed", _host.c_str(), _port);
        return false;
    }
    MIMI_LOGI(TAG, "Connected to SOCKS5 proxy %s:%d", _host.c_str(), _port);

    // Handshake: version 5, 1 method (no auth)
    uint8_t handshake[] = {0x05, 0x01, 0x00};
    sock.write(handshake, 3);

    uint8_t resp[2];
    if (sock.readBytes(resp, 2) != 2 || resp[0] != 0x05 || resp[1] != 0x00) {
        MIMI_LOGE(TAG, "SOCKS5 handshake failed");
        return false;
    }

    // Connect request: version 5, connect, reserved, domain type
    size_t hostLen = strlen(targetHost);
    size_t reqLen = 4 + 1 + hostLen + 2;
    uint8_t* request = (uint8_t*)malloc(reqLen);
    if (!request) return false;

    request[0] = 0x05;  // version
    request[1] = 0x01;  // connect
    request[2] = 0x00;  // reserved
    request[3] = 0x03;  // domain name
    request[4] = (uint8_t)hostLen;
    memcpy(request + 5, targetHost, hostLen);
    request[5 + hostLen] = (uint8_t)(targetPort >> 8);
    request[6 + hostLen] = (uint8_t)(targetPort & 0xFF);
    sock.write(request, reqLen);
    free(request);

    // Read response (at least 10 bytes for IPv4)
    uint8_t connResp[256];
    int n = sock.readBytes(connResp, 10);
    if (n < 10 || connResp[0] != 0x05 || connResp[1] != 0x00) {
        MIMI_LOGE(TAG, "SOCKS5 connect failed: status=%d", n >= 2 ? connResp[1] : -1);
        return false;
    }

    MIMI_LOGI(TAG, "SOCKS5 tunnel established to %s:%d", targetHost, targetPort);
    return true;
}

WiFiClientSecure* MimiProxy::openTLS(const char* host, int port, int timeout_ms) {
    if (!isEnabled()) {
        MIMI_LOGE(TAG, "proxy_conn_open called but no proxy configured");
        return nullptr;
    }

    // First establish plain TCP tunnel through proxy
    WiFiClient* plainSock = new WiFiClient();
    bool ok = false;

    if (_type == "socks5") {
        ok = socks5Handshake(*plainSock, host, port, timeout_ms);
    } else {
        ok = connectTunnel(*plainSock, host, port, timeout_ms);
    }

    if (!ok) {
        delete plainSock;
        return nullptr;
    }

    // Now upgrade to TLS over the tunnel
    // Note: For Arduino ESP32, we use WiFiClientSecure with setInsecure()
    // or proper CA bundle. The tunnel is already established on plainSock.
    WiFiClientSecure* tlsClient = new WiFiClientSecure();
    tlsClient->setInsecure(); // For simplicity; production should use CA bundle
    
    // Transfer the existing TCP connection and do TLS handshake
    // Unfortunately Arduino WiFiClientSecure doesn't support taking over an existing socket easily
    // So we do a direct TLS connection through the tunnel socket
    // For now, we'll note this limitation and provide direct-only TLS
    
    // Actually on ESP32 Arduino, there's no simple way to upgrade an existing socket.
    // Instead, we'll handle proxy at the HTTP level in the LLM module.
    delete plainSock;
    
    // For proxy mode, LLM/Telegram will construct raw HTTP over the tunnel
    MIMI_LOGW(TAG, "TLS-over-proxy requires raw socket handling in calling module");
    delete tlsClient;
    return nullptr;
}

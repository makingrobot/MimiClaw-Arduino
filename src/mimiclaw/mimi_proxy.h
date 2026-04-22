/*
 * MimiClaw-Arduino - HTTP/SOCKS5 Proxy
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_PROXY_H
#define MIMI_PROXY_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "mimi_prefs.h"

class MimiProxy {
public:
    MimiProxy();
    bool begin(MimiPrefs *prefs);
    bool isEnabled();

    void set(const char* host, uint16_t port, const char* type = "http");
    void clear();

    const char* getHost() { return _host.c_str(); }
    uint16_t getPort() { return _port; }
    const char* getType() { return _type.c_str(); }

    /**
     * Open a TLS connection through the proxy to the target host:port.
     * Returns a connected WiFiClientSecure* or nullptr on failure.
     * Caller owns the returned pointer and must delete it.
     */
    WiFiClientSecure* openTLS(const char* host, int port, int timeout_ms = 30000);

private:
    String _host;
    uint16_t _port;
    String _type;   // "http" or "socks5"
    MimiPrefs *_prefs;

    bool connectTunnel(WiFiClient& sock, const char* targetHost, int targetPort, int timeout_ms);
    bool socks5Handshake(WiFiClient& sock, const char* targetHost, int targetPort, int timeout_ms);
};

#endif // MIMI_PROXY_H

/*
 * MimiClaw-Arduino - Web Search
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_WEBSEARCH_H
#define MIMI_WEBSEARCH_H

#include <vector>
#include <Arduino.h>
#include "mimi_tools.h"

class MimiWebsearch {
public:
    MimiWebsearch();
    void init();
    void setBraveKey(const char* key);
    void setTavilyKey(const char* key);
    void setProvider(const char* provider);
    bool search(const char* query, char* output, size_t output_size);

    std::vector<const MimiTool*>& tools() { return _tools; }

    inline const String& provider() const { return _provider; }
    inline const String& brave_key() const { return _brave_key; }
    inline const String& tavily_key() const { return _tavily_key; }

private:
    bool braveSearch(const char* query, char* output, size_t output_size);
    bool tavilySearch(const char* query, char* output, size_t output_size);

    String _provider;
    String _brave_key;
    String _tavily_key;

    std::vector<const MimiTool*> _tools;
};

#endif // MIMI_WEBSEARCH_H



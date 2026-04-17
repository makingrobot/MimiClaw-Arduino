/*
 * MimiClaw - Web Search
 */
#ifndef MIMI_WEBSEARCH_H
#define MIMI_WEBSEARCH_H

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

    void addTools(MimiToolRegistry* registry);

private:
    bool braveSearch(const char* query, char* output, size_t output_size);
    bool tavilySearch(const char* query, char* output, size_t output_size);

    String _provider;
    String _brave_key;
    String _tavily_key;
};

#endif // MIMI_WEBSEARCH_H



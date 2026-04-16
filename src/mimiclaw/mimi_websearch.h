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
    void setKey(const char* key);
    bool search(const char* query, char* output, size_t output_size);

    void addTools(MimiToolRegistry* registry);

private:
    char _search_key[128] = {0};
};

#endif // MIMI_WEBSEARCH_H



/*
 * MimiClaw - Memory Store (SPIFFS-based)
 */
#ifndef MIMI_MEMORY_H
#define MIMI_MEMORY_H

#include <Arduino.h>

class MimiMemory {
public:
    MimiMemory();
    bool begin();

    bool readLongTerm(char* buf, size_t size);
    bool writeLongTerm(const char* content);
    bool appendToday(const char* note);
    bool readRecent(char* buf, size_t size, int days = 3);

private:
    void getDateStr(char* buf, size_t size, int daysAgo);
};

#endif // MIMI_MEMORY_H

/*
 * MimiClaw-Arduino - Memory Store (FILE-based)
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef MIMI_MEMORY_H
#define MIMI_MEMORY_H

#include <Arduino.h>
#include "src/framework/file/file_system.h"

class MimiMemory {
public:
    MimiMemory();
    bool begin(FileSystem *file_system);

    bool readLongTerm(char* buf, size_t size);
    bool writeLongTerm(const char* content);
    bool appendToday(const char* note);
    bool readRecent(char* buf, size_t size, int days = 3);

private:
    void getDateStr(char* buf, size_t size, int daysAgo);
    FileSystem *_file_system;
};

#endif // MIMI_MEMORY_H

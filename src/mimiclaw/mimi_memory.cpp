#include "mimi_memory.h"
#include "mimi_config.h"
#include <time.h>

#define TAG  "memory"

MimiMemory::MimiMemory() {}

bool MimiMemory::begin(FileSystem *file_system) {
    MIMI_LOGI(TAG, "Memory store initialized");
    _file_system = file_system;
    return true;
}

void MimiMemory::getDateStr(char* buf, size_t size, int daysAgo) {
    time_t now;
    time(&now);
    now -= daysAgo * 86400;
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(buf, size, "%Y-%m-%d", &tm);
}

bool MimiMemory::readLongTerm(char* buf, size_t size) {
    File f = _file_system->OpenFile(MIMI_MEMORY_FILE, "r");
    if (!f) {
        buf[0] = '\0';
        return false;
    }
    size_t n = f.readBytes(buf, size - 1);
    buf[n] = '\0';
    f.close();
    return true;
}

bool MimiMemory::writeLongTerm(const char* content) {
    File f = _file_system->OpenFile(MIMI_MEMORY_FILE, "w");
    if (!f) {
        MIMI_LOGE(TAG, "Cannot write %s", MIMI_MEMORY_FILE);
        return false;
    }
    f.print(content);
    f.close();
    MIMI_LOGI(TAG, "Long-term memory updated (%d bytes)", (int)strlen(content));
    return true;
}

bool MimiMemory::appendToday(const char* note) {
    char dateStr[16];
    getDateStr(dateStr, sizeof(dateStr), 0);

    String path = String(MIMI_FILE_MEMORY_DIR) + "/" + dateStr + ".md";

    bool exists = _file_system->ExistsFile(path.c_str());
    File f = _file_system->OpenFile(path.c_str(), exists ? "a" : "w");
    if (!f) {
        MIMI_LOGE(TAG, "Cannot open %s", path.c_str());
        return false;
    }

    if (!exists) {
        f.printf("# %s\n\n", dateStr);
    }
    f.printf("%s\n", note);
    f.close();
    return true;
}

bool MimiMemory::readRecent(char* buf, size_t size, int days) {
    size_t offset = 0;
    buf[0] = '\0';

    for (int i = 0; i < days && offset < size - 1; i++) {
        char dateStr[16];
        getDateStr(dateStr, sizeof(dateStr), i);

        String path = String(MIMI_FILE_MEMORY_DIR) + "/" + dateStr + ".md";
        File f = _file_system->OpenFile(path.c_str(), "r");
        if (!f) continue;

        if (offset > 0 && offset < size - 4) {
            offset += snprintf(buf + offset, size - offset, "\n---\n");
        }

        size_t n = f.readBytes(buf + offset, size - offset - 1);
        offset += n;
        buf[offset] = '\0';
        f.close();
    }

    return true;
}

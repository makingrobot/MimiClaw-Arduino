#include "mimi_context.h"
#include "mimi_config.h"

#define TAG  "context"

static const char DEFAULT_SOUL[] PROGMEM =
    "I am MimiClaw, a personal AI assistant running on an ESP32-S3 microcontroller.\n\n"
    "Personality:\n"
    "- Helpful and friendly\n"
    "- Concise and to the point\n"
    "- Curious and eager to learn\n\n"
    "Values:\n"
    "- Accuracy over speed\n"
    "- User privacy and safety\n"
    "- Transparency in actions\n";

static const char DEFAULT_USER[] PROGMEM =
    "# User Profile\n\n"
    "- Name: (not set)\n"
    "- Language: Chinese / English\n"
    "- Timezone: (not set)\n";


MimiContext::MimiContext() : _memory(nullptr), _skills(nullptr) {

}

size_t MimiContext::appendFile(char* buf, size_t size, size_t offset, const char* path, const char* header) {
    File f = _file_system->OpenFile(path, FILE_READ);
    if (!f) { return offset; }

    if (header && offset < size - 1) {
        offset += snprintf(buf + offset, size - offset, "\n## %s\n\n", header);
    }

    size_t n = f.readBytes(buf + offset, size - offset - 1);
    offset += n;
    buf[offset] = '\0';
    f.close();
    return offset;
}

bool MimiContext::buildSystemPrompt(char* buf, size_t size) {
    size_t off = 0;

    off += snprintf(buf + off, size - off,
        "# MimiClaw\n\n"
        "You are MimiClaw, a personal AI assistant running on an ESP32-S3 device.\n"
        "You communicate through Telegram and WebSocket.\n\n"
        "Be helpful, accurate, and concise.\n\n"
        "## Available Tools\n"
        "You have access to the following tools:\n"
        "- web_search: Search the web for current information. "
        "Use this when you need up-to-date facts, news, weather, or anything beyond your training data.\n"
        "- get_current_time: Get the current date and time. "
        "You do NOT have an internal clock — always use this tool when you need to know the time or date.\n"
        "- read_file: Read a file (path must start with " MIMI_FILE_BASE "/).\n"
        "- write_file: Write/overwrite a file.\n"
        "- edit_file: Find-and-replace edit a file.\n"
        "- list_dir: List files, optionally filter by prefix.\n"
        "- cron_add: Schedule a recurring or one-shot task.\n"
        "- cron_list: List all scheduled cron jobs.\n"
        "- cron_remove: Remove a scheduled cron job by ID.\n\n"
        "When using cron_add for Telegram delivery, always set channel='telegram' and a valid numeric chat_id.\n\n"
        "Use tools when needed. Provide your final answer as text after using tools.\n\n"
        "## Memory\n"
        "You have persistent memory stored on local flash:\n"
        "- Long-term memory: " MIMI_FILE_MEMORY_DIR "/MEMORY.md\n"
        "- Daily notes: " MIMI_FILE_MEMORY_DIR "/daily/<YYYY-MM-DD>.md\n\n"
        "IMPORTANT: Actively use memory to remember things across conversations.\n"
        "- When you learn something new about the user, write it to MEMORY.md.\n"
        "- When something noteworthy happens, append it to today's daily note.\n"
        "- Always read_file MEMORY.md before writing, so you can edit_file without losing content.\n"
        "- Use get_current_time to know today's date before writing daily notes.\n"
        "- Keep MEMORY.md concise and organized.\n"
        "- Proactively save memory without being asked.\n\n"
        "## Skills\n"
        "Skills are specialized instruction files stored in " MIMI_FILE_SKILLS_DIR ".\n"
        "When a task matches a skill, read the full skill file for detailed instructions.\n"
        "You can create new skills using write_file to " MIMI_FILE_SKILLS_DIR "/<name>.md.\n");

    // Bootstrap files    
    if (!_file_system->ExistsFile(MIMI_SOUL_FILE)) {
        _file_system->WriteFile(MIMI_SOUL_FILE, DEFAULT_SOUL); 
        MIMI_LOGI(TAG, "Soul file %s saved.", MIMI_SOUL_FILE); /* file: /config/SOUL.md */
    }
    off = appendFile(buf, size, off, MIMI_SOUL_FILE, "Personality");

    if (!_file_system->ExistsFile(MIMI_USER_FILE)) {
        _file_system->WriteFile(MIMI_USER_FILE, DEFAULT_SOUL); 
        MIMI_LOGI(TAG, "User file %s saved.", MIMI_USER_FILE); /* file: /config/USER.md */
    }
    off = appendFile(buf, size, off, MIMI_USER_FILE, "User Info");

    // Long-term memory
    if (_memory) {
        char memBuf[4096];
        if (_memory->readLongTerm(memBuf, sizeof(memBuf)) && memBuf[0]) {
            off += snprintf(buf + off, size - off, "\n## Long-term Memory\n\n%s\n", memBuf);
        }

        char recentBuf[4096];
        if (_memory->readRecent(recentBuf, sizeof(recentBuf), 3) && recentBuf[0]) {
            off += snprintf(buf + off, size - off, "\n## Recent Notes\n\n%s\n", recentBuf);
        }
    }

    // Skills
    if (_skills) {
        char skillsBuf[2048];
        size_t skillsLen = _skills->buildSummary(skillsBuf, sizeof(skillsBuf));
        if (skillsLen > 0) {
            off += snprintf(buf + off, size - off,
                "\n## Available Skills\n\n"
                "Available skills (use read_file to load full instructions):\n%s\n",
                skillsBuf);
        }
    }

    MIMI_LOGI(TAG, "System prompt built: %d bytes", (int)off);
    return true;
}

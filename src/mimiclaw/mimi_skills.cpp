#include "mimi_skills.h"
#include "mimi_config.h"

#define TAG  "skills"

// ── Built-in skill contents ─────────────────────────────────

static const char BUILTIN_WEATHER[] PROGMEM =
    "# Weather\n\n"
    "Get current weather and forecasts using web_search.\n\n"
    "## When to use\n"
    "When the user asks about weather, temperature, or forecasts.\n\n"
    "## How to use\n"
    "1. Use get_current_time to know the current date\n"
    "2. Use web_search with a query like \"weather in [city] today\"\n"
    "3. Extract temperature, conditions, and forecast from results\n"
    "4. Present in a concise, friendly format\n";

static const char BUILTIN_DAILY_BRIEFING[] PROGMEM =
    "# Daily Briefing\n\n"
    "Compile a personalized daily briefing for the user.\n\n"
    "## When to use\n"
    "When the user asks for a daily briefing, morning update, or \"what's new today\".\n\n"
    "## How to use\n"
    "1. Use get_current_time for today's date\n"
    "2. Read " MIMI_SPIFFS_MEMORY_DIR "/MEMORY.md for user preferences\n"
    "3. Read today's daily note if it exists\n"
    "4. Use web_search for relevant news based on user interests\n"
    "5. Compile a concise briefing\n\n"
    "## Format\n"
    "Keep it brief — 5-10 bullet points max. Use the user's preferred language.\n";

static const char BUILTIN_SKILL_CREATOR[] PROGMEM =
    "# Skill Creator\n\n"
    "Create new skills for MimiClaw.\n\n"
    "## When to use\n"
    "When the user asks to create a new skill or add a capability.\n\n"
    "## How to create a skill\n"
    "1. Choose a short, descriptive name\n"
    "2. Write a SKILL.md file with: Title, Description, When to use, How to use\n"
    "3. Save to " MIMI_SKILLS_PREFIX "<name>.md using write_file\n"
    "4. The skill will be available after the next conversation\n\n"
    "## Best practices\n"
    "- Keep skills concise\n"
    "- Focus on WHAT to do, not HOW\n"
    "- Include specific tool calls the agent should use\n";

struct BuiltinSkill {
    const char* filename;
    const char* content;
};

static const BuiltinSkill s_builtins[] = {
    { "weather",        BUILTIN_WEATHER },
    { "daily-briefing", BUILTIN_DAILY_BRIEFING },
    { "skill-creator",  BUILTIN_SKILL_CREATOR },
};

#define NUM_BUILTINS (sizeof(s_builtins) / sizeof(s_builtins[0]))

MimiSkills::MimiSkills() {}

void MimiSkills::installBuiltin(const char* filename, const char* content) {
    String path = String(MIMI_SKILLS_PREFIX) + filename + ".md";

    if (_file_system->ExistsFile(path.c_str())) {
        MIMI_LOGD(TAG, "Skill exists: %s", path.c_str());
        return;
    }

    File f = _file_system->OpenFile(path.c_str(), "w");
    if (!f) {
        MIMI_LOGE(TAG, "Cannot write skill: %s", path.c_str());
        return;
    }

    f.print(content);
    f.close();
    MIMI_LOGI(TAG, "Installed built-in skill: %s", path.c_str());
}

bool MimiSkills::begin(FileSystem *file_system) {
    MIMI_LOGI(TAG, "Initializing skills system");

    _file_system = file_system;

    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        installBuiltin(s_builtins[i].filename, s_builtins[i].content);
    }

    MIMI_LOGI(TAG, "Skills system ready (%d built-in)", (int)NUM_BUILTINS);
    return true;
}

size_t MimiSkills::buildSummary(char* buf, size_t size) {
    // Enumerate SPIFFS files matching the skills prefix
    File root = _file_system->OpenFile("/");
    if (!root) {
        MIMI_LOGW(TAG, "Cannot open SPIFFS root");
        buf[0] = '\0';
        return 0;
    }

    String prefix = String(MIMI_SKILLS_PREFIX);
    // Remove MIMI_SPIFFS_BASE from prefix for matching SPIFFS filenames
    // SPIFFS filenames are like "/skills/weather.md"
    String matchPrefix = prefix;
    if (matchPrefix.startsWith(MIMI_SPIFFS_BASE)) {
        matchPrefix = matchPrefix.substring(strlen(MIMI_SPIFFS_BASE));
    }

    size_t off = 0;
    File entry = root.openNextFile();

    while (entry && off < size - 1) {
        String name = entry.name();
        //  returns names like "/skills/weather.md"
        if (!name.startsWith(matchPrefix) || !name.endsWith(".md")) {
            entry = root.openNextFile();
            continue;
        }

        // Read first line for title
        String firstLine = entry.readStringUntil('\n');
        firstLine.trim();
        String title = firstLine;
        if (title.startsWith("# ")) title = title.substring(2);

        // Read description (until blank line or next header)
        String desc = "";
        while (entry.available()) {
            String line = entry.readStringUntil('\n');
            line.trim();
            if (line.length() == 0 || line.startsWith("##")) break;
            if (desc.length() > 0) desc += " ";
            desc += line;
        }

        // Build full path for read_file reference
        String fullPath = String(MIMI_SPIFFS_BASE) + name;

        off += snprintf(buf + off, size - off,
            "- **%s**: %s (read with: read_file %s)\n",
            title.c_str(), desc.c_str(), fullPath.c_str());

        entry = root.openNextFile();
    }

    buf[off] = '\0';
    MIMI_LOGI(TAG, "Skills summary: %d bytes", (int)off);
    return off;
}

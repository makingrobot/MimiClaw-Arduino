/*
 * MimiClaw - Skill Loader
 */
#ifndef MIMI_SKILLS_H
#define MIMI_SKILLS_H

#include <Arduino.h>
#include "src/framework/file/file_system.h"

struct SkillInfo {
    const char* filename;
    const char* content;
};

class MimiSkills {
public:
    MimiSkills();
    bool begin(FileSystem *file_system);

    /**
     * Build a summary of all installed skills for the system prompt.
     * @param buf   Output buffer
     * @param size  Buffer size
     * @return      Number of bytes written
     */
    size_t buildSummary(char* buf, size_t size);
    void installSkill(const SkillInfo* info);

private:
    void installBuiltin(const char* filename, const char* content);

    FileSystem *_file_system;
};

#endif // MIMI_SKILLS_H

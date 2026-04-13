/*
 * MimiClaw - Skill Loader
 */
#ifndef MIMI_SKILLS_H
#define MIMI_SKILLS_H

#include <Arduino.h>

class MimiSkills {
public:
    MimiSkills();
    bool begin();

    /**
     * Build a summary of all installed skills for the system prompt.
     * @param buf   Output buffer
     * @param size  Buffer size
     * @return      Number of bytes written
     */
    size_t buildSummary(char* buf, size_t size);

private:
    void installBuiltin(const char* filename, const char* content);
};

#endif // MIMI_SKILLS_H

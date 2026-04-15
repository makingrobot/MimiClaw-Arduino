/*
 * MimiClaw - Context Builder (System Prompt)
 */
#ifndef MIMI_CONTEXT_H
#define MIMI_CONTEXT_H

#include <Arduino.h>
#include "mimi_memory.h"
#include "mimi_skills.h"
#include "src/framework/file/file_system.h"

class MimiContext {
public:
    MimiContext();
    
    void setMemory(MimiMemory* mem) { _memory = mem; }
    void setSkills(MimiSkills* skills) { _skills = skills; }
    void setFileSystem(FileSystem* file_system) { _file_system = file_system; }
    
    /**
     * Build the full system prompt into buf.
     * @return true on success
     */
    bool buildSystemPrompt(char* buf, size_t size);

private:
    MimiMemory* _memory;
    MimiSkills* _skills;
    FileSystem* _file_system;

    size_t appendFile(char* buf, size_t size, size_t offset, const char* path, const char* header);
};

#endif // MIMI_CONTEXT_H

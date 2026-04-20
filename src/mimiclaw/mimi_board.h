/*
 * MimiClaw-Arduino - board
 *
 * Author: Billy Zhang（billy_zh@126.com）
 */
#ifndef _MIMI_BOARD_H
#define _MIMI_BOARD_H

#include <vector>
#include <Arduino.h>

#include "src/framework/board/board.h"
#include "mimi_tools.h"
#include "mimi_skills.h"

class MimiBoard : public Board {
protected:
    std::vector<const MimiTool*> _tools;
    std::vector<const SkillInfo*> _skills;

public:
    MimiBoard();

    std::vector<const MimiTool*>& tools() { return _tools; }
    std::vector<const SkillInfo*>& skills() { return _skills; }

    static std::vector<uint8_t> allow_pins;
};

#endif //_MIMI_BOARD_H

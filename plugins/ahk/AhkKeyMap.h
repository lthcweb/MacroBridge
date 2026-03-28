/*
 * AhkKeyMap.h  —  AIRKey ↔ AHK v2 键名双向映射
 */
#pragma once
#include "AIR.h"
#include <string>

namespace AhkKeyMap {
    AIR::AIRKey AhkToAIR(const std::string& ahkName); // "LButton" → KEY_MOUSE_LEFT
    std::string AIRToAhk(AIR::AIRKey key);             // KEY_MOUSE_LEFT → "LButton"
}
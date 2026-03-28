/*
 * RazerKeyMap.h  —  雷蛇键名 ↔ AIRKey 双向映射
 *
 * 雷蛇 XML 中的键名使用 Windows 虚拟键名字符串，例如：
 *   "VK_LBUTTON"、"VK_F1"、"VK_RETURN"、"VK_SHIFT" 等
 *
 * 同时也可能出现直接的键名字符串，例如：
 *   "a"、"F1"、"Enter"、"LShift"
 *
 * 本模块统一处理两种格式。
 */

#pragma once

#include "AIR.h"
#include <string>

namespace RazerKeyMap {

// 雷蛇键名（支持 VK_xxx 和普通名称）→ AIRKey
AIR::AIRKey RazerKeyToAIR(const std::string& razerKey);

// AIRKey → 雷蛇键名（VK_xxx 格式，用于 Generate）
std::string AIRKeyToRazer(AIR::AIRKey key);

// 鼠标按键编号（1/2/3/4/5）→ AIRKey
AIR::AIRKey RazerMouseButtonToAIR(int buttonIndex);

// AIRKey（鼠标键）→ 雷蛇按键编号
int AIRKeyToRazerMouseButton(AIR::AIRKey key);

} // namespace RazerKeyMap

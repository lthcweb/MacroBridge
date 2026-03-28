/*
 * LogitechKeyMap.h  —  罗技键名 ↔ AIRKey 双向映射
 *
 * 罗技 Lua 脚本中的键名格式（区分大小写，全小写）：
 *   鼠标按钮：数字 1~5（直接整数，不是字符串）
 *   键盘键名："a" "lalt" "lshift" "lctrl" "f1" 等
 *   特殊："backspace" "tab" "return" "escape" "space"
 *          "delete" "insert" "home" "end" "pageup" "pagedown"
 *          "up" "down" "left" "right"
 *          "numlock" "scrolllock" "capslock" "pause" "printscreen"
 */
#pragma once
#include "AIR.h"
#include <string>

namespace LogitechKeyMap {

// 罗技键名字符串 → AIRKey（大小写不敏感）
AIR::AIRKey LuaKeyToAIR(const std::string& luaKey);

// AIRKey → 罗技键名字符串
std::string AIRKeyToLua(AIR::AIRKey key);

// 罗技鼠标按钮编号（1~5）→ AIRKey
AIR::AIRKey LuaMouseBtnToAIR(int btn);

// AIRKey → 罗技鼠标按钮编号（鼠标键返回 1~5，非鼠标键返回 0）
int AIRKeyToLuaMouseBtn(AIR::AIRKey key);

} // namespace LogitechKeyMap

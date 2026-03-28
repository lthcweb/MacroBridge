/*
 * LogitechPlugin.h  —  罗技 G HUB / LGS Lua 宏插件
 *
 * 罗技宏格式说明（Lua 脚本）：
 *
 *   function OnEvent(event, arg)
 *     if event == "MOUSE_BUTTON_PRESSED" and arg == 1 then
 *       repeat
 *         MoveMouseRelative(0, 3)
 *         Sleep(50)
 *       until not IsMouseButtonPressed(1)
 *     end
 *   end
 *
 * 已知 API：
 *   Sleep(ms)                     延迟
 *   PressKey("keyName")           按键按下
 *   ReleaseKey("keyName")         按键松开
 *   PressAndReleaseKey("keyName") 按键点击
 *   MoveMouseRelative(dx, dy)     鼠标相对移动
 *   MoveMouseTo(x, y)             鼠标绝对移动
 *   PressMouseButton(btn)         鼠标按下（1=左 2=右 3=中）
 *   ReleaseMouseButton(btn)       鼠标松开
 *   MoveMouseWheel(clicks)        滚轮（正=上 负=下）
 *   IsMouseButtonPressed(btn)     查询鼠标键状态 → bool
 *   IsKeyLockOn("keyName")        查询 CapsLock 等锁定键
 *
 * event 字符串（OnEvent 第一参数）：
 *   "MOUSE_BUTTON_PRESSED"  / "MOUSE_BUTTON_RELEASED"
 *   "G_PRESSED"             / "G_RELEASED"
 *   "M_PRESSED"             / "M_RELEASED"
 *   "MACRO_START_EVENT"     / "MACRO_STOP_EVENT"
 *
 * 编译约定：/MD + 相同 VS 版本（见 AIR.h）
 */
#pragma once
#include "AIR.h"

#ifdef _WIN32
#  ifdef LOGITECHPLUGIN_EXPORTS
#    define LOGITECHPLUGIN_API __declspec(dllexport)
#  else
#    define LOGITECHPLUGIN_API __declspec(dllimport)
#  endif
#else
#  define LOGITECHPLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {
    LOGITECHPLUGIN_API AIR::IScriptPlugin* CreateLogitechPlugin();
    LOGITECHPLUGIN_API void                DestroyLogitechPlugin(AIR::IScriptPlugin* p);
    // 通用别名
    LOGITECHPLUGIN_API AIR::IScriptPlugin* CreatePlugin();
    LOGITECHPLUGIN_API void                DestroyPlugin(AIR::IScriptPlugin* p);
}

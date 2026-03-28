/*
 * RazerPlugin.h  —  雷蛇宏插件对外头文件
 *
 * 雷蛇 Synapse 宏格式说明（XML）：
 *
 *   <Macro>
 *     <Name>宏名</Name>
 *     <MacroEvents>
 *       <MacroEvent>
 *         <Type>N</Type>       事件类型（见 RazerEventType 枚举）
 *         <Delay>ms</Delay>    初始延迟
 *         <MouseMovement>      鼠标移动序列（Type=3 时存在）
 *           <MouseMovementEvent>
 *             <Type>3</Type>   移动类型（目前只见过 3=绝对坐标）
 *             <X>875</X>
 *             <Y>84</Y>
 *             <Delay>32</Delay>  到达此点前的延迟（毫秒）
 *           </MouseMovementEvent>
 *           ...
 *         </MouseMovement>
 *         <KeyboardEvent>      键盘事件（Type=1/2 时存在）
 *           <Key>...</Key>     键名（Windows 虚拟键名）
 *         </KeyboardEvent>
 *         <MouseButtonEvent>   鼠标按键事件（Type=4/5 时存在）
 *           <Button>N</Button> 按键编号（1=左 2=右 3=中）
 *         </MouseButtonEvent>
 *       </MacroEvent>
 *       ...
 *     </MacroEvents>
 *   </Macro>
 *
 * 已知 MacroEvent.Type 值：
 *   1 = 键盘按下（KeyboardEvent）
 *   2 = 键盘松开（KeyboardEvent）
 *   3 = 鼠标移动序列（MouseMovement）
 *   4 = 鼠标按键按下（MouseButtonEvent）
 *   5 = 鼠标按键松开（MouseButtonEvent）
 *   6 = 延迟（仅 Delay 字段有效）
 *
 * 编译约定：同 AIR.h 说明，必须 /MD + 相同 VS 版本。
 */

#pragma once

#include "AIR.h"

#ifdef _WIN32
#  ifdef RAZERPLUGIN_EXPORTS
#    define RAZERPLUGIN_API __declspec(dllexport)
#  else
#    define RAZERPLUGIN_API __declspec(dllimport)
#  endif
#else
#  define RAZERPLUGIN_API __attribute__((visibility("default")))
#endif

using CreatePluginFunc  = AIR::IScriptPlugin* (*)();
using DestroyPluginFunc = void (*)(AIR::IScriptPlugin*);

extern "C" {
    RAZERPLUGIN_API AIR::IScriptPlugin* CreateRazerPlugin();
    RAZERPLUGIN_API void DestroyRazerPlugin(AIR::IScriptPlugin* plugin);

    // 通用别名
    RAZERPLUGIN_API AIR::IScriptPlugin* CreatePlugin();
    RAZERPLUGIN_API void DestroyPlugin(AIR::IScriptPlugin* p);

}

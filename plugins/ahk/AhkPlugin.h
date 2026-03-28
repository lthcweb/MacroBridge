/*
 * AhkPlugin.h  —  AutoHotkey v2 插件对外头文件
 *
 * 支持：
 *   Parse    : AHK v2 脚本文本 → AIR 树
 *   Generate : AIR 树 → AHK v2 脚本文本
 *
 * 编译约定：/MD + 相同 VS 版本（见 AIR.h）
 */
#pragma once
#include "AIR.h"

#ifdef _WIN32
#  ifdef AHKPLUGIN_EXPORTS
#    define AHKPLUGIN_API __declspec(dllexport)
#  else
#    define AHKPLUGIN_API __declspec(dllimport)
#  endif
#else
#  define AHKPLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {
    AHKPLUGIN_API AIR::IScriptPlugin* CreateAhkPlugin();
    AHKPLUGIN_API void                DestroyAhkPlugin(AIR::IScriptPlugin* p);
    // 通用别名（供 PluginManager 用统一名称加载）
    AHKPLUGIN_API AIR::IScriptPlugin* CreatePlugin();
    AHKPLUGIN_API void                DestroyPlugin(AIR::IScriptPlugin* p);
}
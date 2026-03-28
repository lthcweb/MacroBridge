/*
 * PluginExportAlias.h
 *
 * 每个插件的 XxxPlugin.cpp 末尾 #include 此文件，
 * 自动生成通用别名 CreatePlugin / DestroyPlugin。
 *
 * PluginManager 通过 GetProcAddress("CreatePlugin") 统一加载，
 * 无需知道每个插件的具体函数名。
 *
 * 用法（在 XxxPlugin.cpp 末尾）：
 *
 *   // 定义插件具体的工厂函数名和类名
 *   #define PLUGIN_CREATE_FUNC   CreateAhkPlugin
 *   #define PLUGIN_DESTROY_FUNC  DestroyAhkPlugin
 *   #define PLUGIN_CLASS         AhkPlugin
 *   #define PLUGIN_API           AHKPLUGIN_API
 *   #include "PluginExportAlias.h"
 */

#pragma once

// 确保这三个宏都已定义
#ifndef PLUGIN_CREATE_FUNC
#  error "请在 #include PluginExportAlias.h 之前定义 PLUGIN_CREATE_FUNC"
#endif
#ifndef PLUGIN_DESTROY_FUNC
#  error "请在 #include PluginExportAlias.h 之前定义 PLUGIN_DESTROY_FUNC"
#endif
#ifndef PLUGIN_API
#  error "请在 #include PluginExportAlias.h 之前定义 PLUGIN_API"
#endif

extern "C" {

// 通用创建接口（PluginManager 使用此名称）
PLUGIN_API AIR::IScriptPlugin* CreatePlugin() {
    return PLUGIN_CREATE_FUNC();
}

// 通用销毁接口
PLUGIN_API void DestroyPlugin(AIR::IScriptPlugin* p) {
    PLUGIN_DESTROY_FUNC(p);
}

} // extern "C"

// 用完即清，避免污染
#undef PLUGIN_CREATE_FUNC
#undef PLUGIN_DESTROY_FUNC
#undef PLUGIN_API

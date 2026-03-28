/*
 * PluginManager.h  —  插件加载与管理
 *
 * 职责：
 *   - 扫描 exe 同目录下所有 *Plugin.dll
 *   - LoadLibrary + GetProcAddress 获取工厂函数
 *   - 统一管理插件生命周期（Create / Destroy）
 *   - 对外暴露插件列表，供 UI 层填充"源格式"/"目标格式"下拉框
 *
 * 插件约定：
 *   每个 DLL 必须导出以下两个 C 函数（extern "C"）：
 *     AIR::IScriptPlugin* Create<PluginName>Plugin()
 *     void                Destroy<PluginName>Plugin(AIR::IScriptPlugin*)
 *
 *   但为了让 PluginManager 无需知道具体名称，
 *   每个 DLL 还必须导出通用别名：
 *     AIR::IScriptPlugin* CreatePlugin()
 *     void                DestroyPlugin(AIR::IScriptPlugin*)
 *
 *   见各插件的 .def 文件或 PLUGIN_EXPORT_ALIASES 宏（PluginHelpers.cmake 提供）。
 */

#pragma once

#include "AIR.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "ThirdParty/Singleton/Singleton.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// ============================================================================
//  PluginEntry：一个已加载的插件条目
// ============================================================================
namespace {
using DestroyPluginFn = void (*)(AIR::IScriptPlugin*);

struct PluginEntry {
    std::string             path;       // DLL 完整路径
    std::string             name;       // GetFormatName() 返回值
    std::string             extension;  // GetFileExtension() 返回值
    AIR::IScriptPlugin*     plugin = nullptr;
    DestroyPluginFn         destroyFn = nullptr;

#ifdef _WIN32
    HMODULE                 hModule = nullptr;
#endif

    bool canParse()    const { return plugin && plugin->CanParse();    }
    bool canGenerate() const { return plugin && plugin->CanGenerate(); }
};
}
// ============================================================================
//  PluginManager
// ============================================================================

class PluginManager : public Singleton<PluginManager> {
public:
    ~PluginManager() { unloadAll();    }

    /**
     * 扫描指定目录下所有 *Plugin.dll 并加载。
     *
     * @param pluginDir  扫描目录，默认为空字符串（表示 exe 所在目录）
     * @return           成功加载的插件数量
     */
    int loadAll(const std::string& pluginDir = {});

    /**
     * 加载单个插件 DLL。
     *
     * @param dllPath  DLL 完整路径
     * @return         成功返回 true
     */
    bool loadOne(const std::string& dllPath);

    /**
     * 卸载所有已加载的插件。
     */
    void unloadAll();

    /**
     * 获取所有已加载的插件列表。
     */
    const std::vector<PluginEntry>& plugins() const { return m_plugins; }

    /**
     * 按格式名查找插件（大小写不敏感）。
     * 返回 nullptr 表示未找到。
     */
    AIR::IScriptPlugin* findByName(const std::string& formatName) const;

    /**
     * 按文件扩展名查找插件（如 ".ahk"、".xml"）。
     * 返回 nullptr 表示未找到。
     */
    AIR::IScriptPlugin* findByExtension(const std::string& ext) const;

	void SetSourcePlugin(AIR::IScriptPlugin* plugin) { m_sourcePlugin = plugin; }
	void SetTargetPlugin(AIR::IScriptPlugin* plugin) { m_targetPlugin = plugin; }

	AIR::IScriptPlugin* GetSourcePlugin() const { return m_sourcePlugin; }
	AIR::IScriptPlugin* GetTargetPlugin() const { return m_targetPlugin; }

    /**
     * 获取 exe 所在目录的完整路径（含尾部路径分隔符）。
     */
    static std::string exeDir();

protected:
    void OnInit() override {
        std::cout << "[PluginManager] OnInit() this = " << this
            << ", size = " << m_plugins.size() << std::endl;
    };

private:
    std::vector<PluginEntry> m_plugins;
	AIR::IScriptPlugin* m_sourcePlugin = nullptr;
	AIR::IScriptPlugin* m_targetPlugin = nullptr;
};

/*
 * PluginManager.cpp  —  插件加载实现
 */

#include "PluginManager.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>    // PathCombineA
#pragma comment(lib, "shlwapi.lib")
#endif

// ============================================================================
//  工厂函数类型（与各插件头文件中的声明一致）
// ============================================================================
using CreatePluginFn  = AIR::IScriptPlugin* (*)();
using DestroyPluginFn = void (*)(AIR::IScriptPlugin*);

// ============================================================================
//  exeDir()：获取 exe 所在目录
// ============================================================================

std::string PluginManager::exeDir()
{
#ifdef _WIN32
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    // 去掉文件名，保留目录部分
    char* lastSep = strrchr(buf, '\\');
    if (lastSep) *(lastSep + 1) = '\0';
    return buf;
#else
    // Linux / macOS（后续扩展用）
    return "./";
#endif
}

// ============================================================================
//  loadAll()：扫描目录，加载所有 *Plugin.dll
// ============================================================================

int PluginManager::loadAll(const std::string& pluginDir)
{
    std::string dir = pluginDir.empty() ? exeDir() : pluginDir;

    // 确保以路径分隔符结尾
    if (!dir.empty() && dir.back() != '\\' && dir.back() != '/')
        dir += '\\';

#ifdef _WIN32
    // 枚举目录下所有 *Plugin.dll
    std::string pattern = dir + "*Plugin.dll";
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &ffd);

    if (hFind == INVALID_HANDLE_VALUE) {
        // 目录下没有插件，不算错误
        return 0;
    }

    int loaded = 0;
    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string fullPath = dir + ffd.cFileName;
            if (loadOne(fullPath))
                ++loaded;
        }
    } while (FindNextFileA(hFind, &ffd));

    FindClose(hFind);
    return loaded;
#else
    // 非 Windows 留空，后续扩展
    return 0;
#endif
}

// ============================================================================
//  loadOne()：加载单个插件 DLL
// ============================================================================

bool PluginManager::loadOne(const std::string& dllPath)
{
#ifdef _WIN32
    HMODULE hMod = LoadLibraryA(dllPath.c_str());
    if (!hMod) {
        DWORD err = GetLastError();
        std::cerr << "[PluginManager] LoadLibrary 失败: "
                  << dllPath << "  错误码: " << err << "\n";
        return false;
    }

    // 查找通用工厂函数 CreatePlugin
    auto createFn  = reinterpret_cast<CreatePluginFn>(
                        GetProcAddress(hMod, "CreatePlugin"));
    auto destroyFn = reinterpret_cast<DestroyPluginFn>(
                        GetProcAddress(hMod, "DestroyPlugin"));

    if (!createFn || !destroyFn) {
        std::cerr << "[PluginManager] DLL 未导出 CreatePlugin/DestroyPlugin: "
                  << dllPath << "\n";
        FreeLibrary(hMod);
        return false;
    }

    // 创建插件实例
    AIR::IScriptPlugin* plugin = nullptr;
    try {
        plugin = createFn();
    } catch (...) {
        std::cerr << "[PluginManager] CreatePlugin() 抛出异常: "
                  << dllPath << "\n";
        FreeLibrary(hMod);
        return false;
    }

    if (!plugin) {
        std::cerr << "[PluginManager] CreatePlugin() 返回 nullptr: "
                  << dllPath << "\n";
        FreeLibrary(hMod);
        return false;
    }

    PluginEntry entry;
    entry.path      = dllPath;
    entry.name      = plugin->GetFormatName();
    entry.extension = plugin->GetFileExtension();
    entry.plugin    = plugin;
    entry.hModule   = hMod;

    m_plugins.push_back(std::move(entry));

/*    std::cout << "[PluginManager] 已加载插件: "
              << m_plugins.back().name << "  (" << dllPath << ")\n";*/
    return true;

#else
    return false;
#endif
}

// ============================================================================
//  unloadAll()
// ============================================================================

void PluginManager::unloadAll()
{
#ifdef _WIN32
    for (auto& entry : m_plugins) {
        if (entry.plugin && entry.hModule) {
            // 通过 DLL 的 DestroyPlugin 销毁实例（确保在同一堆上释放）
            auto destroyFn = reinterpret_cast<DestroyPluginFn>(
                GetProcAddress(entry.hModule, "DestroyPlugin"));
            if (destroyFn)
                destroyFn(entry.plugin);
            FreeLibrary(entry.hModule);
        }
    }
#endif
    m_plugins.clear();
}

// ============================================================================
//  查找
// ============================================================================

AIR::IScriptPlugin* PluginManager::findByName(const std::string& formatName) const
{
    for (const auto& e : m_plugins) {
        std::string a = e.name, b = formatName;
        auto toLow = [](std::string& s) {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c){ return (char)std::tolower(c); });
        };
        toLow(a); toLow(b);
        if (a == b) return e.plugin;
    }
    return nullptr;
}

AIR::IScriptPlugin* PluginManager::findByExtension(const std::string& ext) const
{
    for (const auto& e : m_plugins) {
        std::string a = e.extension, b = ext;
        auto toLow = [](std::string& s) {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c){ return (char)std::tolower(c); });
        };
        toLow(a); toLow(b);
        if (a == b) return e.plugin;
    }
    return nullptr;
}

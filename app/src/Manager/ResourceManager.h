#pragma once

#include <string>
#include "ThirdParty/Singleton/Singleton.hpp"

namespace DolPP {

class ResourceManager : public Singleton<ResourceManager>
{
public:
    // Initialize resource roots from current executable directory.
    // This should be called once at app startup.
    bool Initialize();

    // Current resolved paths (Windows style).
    const std::wstring& GetAppRootDir() const { return m_appRootDir; }
    const std::wstring& GetConfigDir() const { return m_configDir; }
    const std::wstring& GetSettingsPath() const { return m_settingsPath; }

    // Reserved for future extension.
    const std::wstring& GetSkinRootDir() const { return m_skinRootDir; }
    const std::wstring& GetLanguageDir() const { return m_languageDir; }
    const std::wstring& GetScriptsDir() const { return m_scriptsDir; }

protected:
    void OnInit() override;

private:
    static bool EnsureDir(const std::wstring& dir);

private:
    std::wstring m_appRootDir;
    std::wstring m_configDir;
    std::wstring m_settingsPath;

    // Future resources.
    std::wstring m_skinRootDir;
    std::wstring m_languageDir;
    std::wstring m_scriptsDir;
};

} // namespace DolPP

#include "ResourceManager.h"

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

namespace DolPP {

void ResourceManager::OnInit()
{
    // Lazy singleton: no heavy work here.
}

bool ResourceManager::EnsureDir(const std::wstring& dir)
{
    if (PathFileExistsW(dir.c_str())) {
        return true;
    }
    return ::CreateDirectoryW(dir.c_str(), nullptr) == TRUE;
}

bool ResourceManager::Initialize()
{
    wchar_t szPath[MAX_PATH] = { 0 };
    if (::GetModuleFileNameW(nullptr, szPath, MAX_PATH) == 0) {
        return false;
    }

    ::PathRemoveFileSpecW(szPath);
    m_appRootDir = szPath;

    m_configDir = m_appRootDir + L"\\config";
    if (!EnsureDir(m_configDir)) {
        return false;
    }
    m_settingsPath = m_configDir + L"\\settings.ini";

    // Reserved for future expansion.
    m_skinRootDir = m_appRootDir + L"\\skin";
    // language 目录不需要根目录，Duilib 会组装 CPaintManagerUI::GetInstancePath()
    m_languageDir = L"language\\";
    m_scriptsDir = m_appRootDir + L"\\scripts";

    return true;
}

} // namespace DolPP

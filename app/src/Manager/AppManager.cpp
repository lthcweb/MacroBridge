#include "AppManager.h"

#include <iostream>
#include <UIlib.h>

#include "PluginManager.h"
#include "ResourceManager.h"

//#include "SkinManager.h"
//#include "SettingsManager.h"
//#include "EventManager.h"
//#include "../Script/LuaManager.h"
//#include "Common/Utils/string_conv.h"

using namespace DuiLib;

namespace DolPP
{

/////////////////////////////////////////////////////////////////////////////////////
//
//

int AppManager::Run(HINSTANCE hInstance, int nCmdShow)
{
	m_nCmdShow = nCmdShow;

	// 初始化UI管理器
	CPaintManagerUI::SetInstance(hInstance);

	// 初始化资源
	InitResource();

	// 载入插件
	LoadPlugins();

	// 启动主窗口
	LoadMainWin();

	CPaintManagerUI::MessageLoop();

	// 销毁窗口
	if (m_pMainWnd) {
		delete m_pMainWnd;
		m_pMainWnd = NULL;
	}

	// 清理资源
	CPaintManagerUI::Term();

	return 0;
}

void AppManager::OnInit()
{

}

void AppManager::InitResource(const CDuiString& resourcePath)
{
	ResourceManager::GetInstance().Initialize();
/*	SkinManager::GetInstance().Initialize(ResourceManager::GetInstance().GetSkinRootDir());
	SettingsManager::GetInstance().SetConfigPath(ResourceManager::GetInstance().GetSettingsPath());
	SettingsManager::GetInstance().Load();

	auto theme = SettingsManager::GetInstance().GetTheme();
	if (!SkinManager::GetInstance().SetCurrentSkin(theme)) {
		SkinManager::GetInstance().SetCurrentSkin(resourcePath.GetData());
	}

	LuaManager::GetInstance().SetScriptsRootDir(Utils::ToString(ResourceManager::GetInstance().GetScriptsDir()));
*/
	CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());
	return;

}

void AppManager::TrimProcessMemory()
{
	HANDLE hProc = GetCurrentProcess();
	// 修剪工作集
	SetProcessWorkingSetSize(hProc, (SIZE_T)-1, (SIZE_T)-1);
	// 压缩默认堆（可选）
	HeapCompact(GetProcessHeap(), 0);
	// 提示 Segment Heap 清理内部缓存（Win10+）
	HeapSetInformation(NULL, HeapOptimizeResources, NULL, 0);

}
void AppManager::LoadPlugins()
{
	int count = PluginManager::GetInstance().loadAll(); // 扫描 exe 同目录下所有 *Plugin*.dll
	std::cout << "已加载插件数量: " << count << "\n";

	for (const auto& entry : PluginManager::GetInstance().plugins()) {
		std::cout << "  [" << entry.name << "]"
			<< "  扩展名: " << entry.extension
			<< "  Parse: " << (entry.canParse() ? "Y" : "N")
				<< "  Generate: " << (entry.canGenerate() ? "Y" : "N")
				<< "\n";
	}
}
void AppManager::LoadMainWin()
{
		// 载入配置文件
	//SettingsManager::GetInstance().Load();

	m_pMainWnd = new MainWindow();
	if (!m_pMainWnd)
		return;

	DWORD style = WS_POPUP;   // 无边框
	DWORD exstyle = WS_EX_APPWINDOW;  // 强制显示在应用列表

	m_pMainWnd->Create(nullptr, _T("宏桥"), UI_WNDSTYLE_FRAME, 0L, 0, 0, 800, 600);
	//m_pMainWnd->CenterWindow();
	m_pMainWnd->ShowWindow(m_nCmdShow);



}
} // namespace DolPP

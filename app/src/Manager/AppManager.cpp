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
	int count = PluginManager::GetInstance().loadAll(); // 扫描 exe 同目录下所有 *Plugin.dll
	std::cout << "已加载插件数量: " << count << "\n";

	for (const auto& entry : PluginManager::GetInstance().plugins()) {
		std::cout << "  [" << entry.name << "]"
			<< "  扩展名: " << entry.extension
			<< "  Parse: " << (entry.canParse() ? "Y" : "N")
			<< "  Generate: " << (entry.canGenerate() ? "Y" : "N")
			<< "\n";
	}

	// ── 演示转换流程（控制台测试用，UI 实现后删除）────────────────────────────
	// 假设已有一个 .ahk 文件内容：
	const std::string ahkSource = R"(
; 这是一个简单的 AHK 脚本示例，演示了热键和鼠标操作
~LButton::
{
    while (GetKeyState("LButton", "P")) {
        MouseMove 0, 3, 0, "R"
        Sleep Random(20, 30)
    }
}
)";

	auto* ahkPlugin = PluginManager::GetInstance().findByExtension(".ahk");
	if (ahkPlugin) {
		std::vector<AIR::AIRDiagnostic> diags;

		// Parse：AHK 文本 → AIR 树
		auto airRoot = ahkPlugin->Parse(ahkSource, diags);
		if (airRoot) {
			std::cout << "\n[Parse 成功] 诊断数量: " << diags.size() << "\n";

			// Generate：AIR 树 → AHK 文本（往返测试）
			diags.clear();
			std::string output = ahkPlugin->Generate(*airRoot, diags);
			std::cout << "\n[Generate 输出]\n" << output << "\n";

			// Razer 测试
			auto* razerPlugin = PluginManager::GetInstance().findByExtension(".xml");
			output = razerPlugin->Generate(*airRoot, diags);
			std::cout << "\n[Generate 输出]\n" << output << "\n";

			// Logitech 测试
			auto* logiPlugin = PluginManager::GetInstance().findByExtension(".lua");
			output = logiPlugin->Generate(*airRoot, diags);
			std::cout << "\n[Generate 输出]\n" << output << "\n";

		}
		else {
			std::cout << "[Parse 失败]\n";
			for (const auto& d : diags)
				std::cout << "  ERROR: " << d.message << "\n";
		}
	}

	// ── 启动 UI（待 DuiLib 集成后替换以下代码）────────────────────────────────
/*	MessageBoxW(nullptr,
		L"MacroBridge\n插件加载完成，UI 待实现。",
		L"MacroBridge",
		MB_OK | MB_ICONINFORMATION);*/
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

#include "MainWindow.h"
/*#include <shellapi.h>
#include "Common/Utils/string_conv.h"
#include "../../../Manager/SettingsManager.h"
#include "../../../Manager/EventManager.h"
#include "../../../Manager/SkinManager.h"*/
#include "../Views/MainWindowUI.h"


namespace DolPP
{

// 1. 定义消息 ID
#define WM_ROCKET_SIGNAL_INCOMING (WM_USER + 1002)

MainWindow::MainWindow()
{
	m_pPaintManager = &m_pm;
}

MainWindow::~MainWindow()
{
}

void MainWindow::OnFinalMessage(HWND hWnd)
{
	WindowImplBase::OnFinalMessage(hWnd);
}



// ------------------------------------------------------------
// 多语言接口
// ------------------------------------------------------------
LPCTSTR MainWindow::QueryControlText(LPCTSTR lpstrId, LPCTSTR lpstrType)
{
	return _T("");
}

void MainWindow::InitWindow()
{
	m_pMaxBtn = static_cast<CButtonUI*>(m_pPaintManager->FindControl(_T("main_title_btn_maximize")));
	m_pRestoreBtn = static_cast<CButtonUI*>(m_pPaintManager->FindControl(_T("main_title_btn_restore")));

	m_pMainWindowUI = UIPageBase::CreatePage <MainWindowUI>(m_pPaintManager);

/*	// 设置信号回调函数，用于通知消息队列分发
	// 和 HandleCustomMessage(...) 配合使用
	EventManager::GetInstance().SetNotifyHandler([hWnd = this->m_hWnd]() {
		::PostMessage(hWnd, WM_ROCKET_SIGNAL_INCOMING, 0, 0);
	});

	// 连接所有信号
	EventManager::GetInstance().ConnectAll();*/

/*	LoadConfig();
	LoadLanguage();*/

}

// ------------------------------------------------------------
// UI 消息分发
// ------------------------------------------------------------
void MainWindow::Notify(TNotifyUI& msg)
{
}

LRESULT MainWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM, BOOL& bHandled)
{
	// 默认不处理
	bHandled = FALSE;

	switch (uMsg) {

	case WM_DESTROY:	// 关闭窗口，退出程序
	{
		::PostQuitMessage(0L);
		bHandled = TRUE;
		return 0;
	}

	case WM_EXITSIZEMOVE:
	{
		RECT rc;
		::GetWindowRect(m_hWnd, &rc);
/*		SettingsManager::GetInstance().SetWindowRect(rc);
		SettingsManager::GetInstance().Save();*/

		bHandled = TRUE;
		return 0;
	}

	case WM_SIZE:
	{
		if (wParam == SIZE_MAXIMIZED) {
			// 窗口最大化了
			UpdateMaxButtonState(true);
		}
		else if (wParam == SIZE_RESTORED) {
			// 窗口恢复正常了（包括拖动恢复）
			UpdateMaxButtonState(false);
		}
		else  if (wParam == SIZE_MINIMIZED) {
			TrimProcessMemory();
		}
		break;
	}

	case WM_ROCKET_SIGNAL_INCOMING: // 处理 Rocket 内部的消息队列
	{
/*		EventManager::GetInstance().DispatchQueuedCalls();*/
		bHandled = TRUE;
		return 0;
	}

	case WM_CLOSE: 
	{
		return 0;
	}
	default:
	{
		break;
	}

	}

	// 其他不关心的消息，全部交给 return 0，系统会处理
	return 0;
}




void MainWindow::UpdateMaxButtonState(bool bMaximized)
{
	if (!m_pMaxBtn || !m_pRestoreBtn)
		return;

	if (bMaximized) {
		// 改为恢复图标
		m_pMaxBtn->SetVisible(false);// SetAttribute(_T("normalimage"), _T("file='icons/restore.png'"));
		m_pRestoreBtn->SetVisible(true);
	}
	else {
		// 改为最大化图标
		m_pMaxBtn->SetVisible(true);
		m_pRestoreBtn->SetVisible(false);
		//pMaxBtn->SetAttribute(_T("normalimage"), _T("file='icons/max.png'"));
	}
}

void MainWindow::TrimProcessMemory()
{
	// 1. 应用层：先通知你的业务模块清理无用缓存
	// 例如：Lua 垃圾回收
	// LuaManager::GetInstance().GC(); 
	// 或者：EventManager::GetInstance().ClearUnusedConnections();

	// 2. 堆压缩 (Heap Compact)
	// 只有在空闲（最小化）时做，因为它比较耗时
	HANDLE hHeap = GetProcessHeap();
	if (hHeap) {
		HeapCompact(hHeap, 0);
	}

	// 3. 系统级优化 (Win10 1903+)
	// 这是目前最推荐的方式，它不会暴力置换内存，而是让系统尝试回收不再使用的内存块
	// 注意：HeapOptimizeResources 需要在 HeapSetInformation 中正确调用
	HEAP_OPTIMIZE_RESOURCES_INFORMATION heapOptInfo;
	heapOptInfo.Version = HEAP_OPTIMIZE_RESOURCES_CURRENT_VERSION;
	heapOptInfo.Flags = 0;
	HeapSetInformation(NULL, HeapOptimizeResources, &heapOptInfo, sizeof(heapOptInfo));

	// 4. 工作集清理 (谨慎使用)
	// 只有当程序真的处于“挂机/后台”模式且很长一段时间不操作时才调这个
	HANDLE hProc = GetCurrentProcess();
	SetProcessWorkingSetSize(hProc, (SIZE_T)-1, (SIZE_T)-1);

	// 5. 释放多余的 DLL 引用
	// 强制系统卸载不再使用的库（有些库加载后即便不再使用也会占位）
	CoFreeUnusedLibraries();
}

void MainWindow::Log(LPCTSTR text)
{
	// 你的 logBox 之后再补
}

} // namespace DolPP

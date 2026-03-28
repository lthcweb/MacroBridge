#pragma once
#include <UIlib.h>
//#include "../../../Common/Event.h"

using namespace DuiLib;
namespace DolPP
{

/////////////////////////////////////////////////////////////////////////////////////
//
//

class MainWindowUI;

/// 主窗口类
class MainWindow : public WindowImplBase
{
public:
	MainWindow();
	~MainWindow();

public: // WindowImplBase override
	LPCTSTR GetWindowClassName() const override { return _T("CMainWnd"); }
	CDuiString GetSkinFile() override { return _T("main.xml"); }
	//UILIB_RESTYPE GetResourceType() const override { return UILIB_FILE; }
	void InitWindow() override;    // XML 已加载完毕
	void OnFinalMessage(HWND hWnd) override;

public: // 控件扩展
	//CControlUI* CreateControl(LPCTSTR pstrClass) override;
	LPCTSTR QueryControlText(LPCTSTR lpstrId, LPCTSTR lpstrType) override;
   // BOOL Receive(SkinChangedParam param) override;
	LRESULT HandleCustomMessage(UINT uMsg, WPARAM, LPARAM, BOOL& bHandled) override;

public: // UI 通知
	void Notify(TNotifyUI& msg) override;


private: // 自己的业务函数

	void UpdateMaxButtonState(bool bMaximized);
	void TrimProcessMemory();
	void Log(LPCTSTR text);

private:
	CPaintManagerUI* m_pPaintManager;
	// UI 变量
	CButtonUI* m_pCloseBtn = nullptr;
	CButtonUI* m_pMaxBtn = nullptr;
	CButtonUI* m_pRestoreBtn = nullptr;
	CButtonUI* m_pMinBtn = nullptr;
	CButtonUI* m_pSkinBtn = nullptr;


	MainWindowUI* m_pMainWindowUI = nullptr;

	bool m_bUserMoving = false;

	CMenuWnd* m_pMenu = nullptr;
	CTrayIcon m_trayIcon;

};

} // namespace DolPP

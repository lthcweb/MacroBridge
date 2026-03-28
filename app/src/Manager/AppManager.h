#pragma once

#include "ThirdParty/Singleton/Singleton.hpp"
#include "../UI/Main/Windows/MainWindow.h"
//#include "../UI/AuthCenter/UIManager.h"


namespace DolPP
{
//namespace Billing
//{
//struct UISignInResult;
//class BillingSession;
//}
/////////////////////////////////////////////////////////////////////////////////////
//
//

class AppManager : public Singleton<AppManager>
{
public:
	int Run(HINSTANCE hInstance, int nCmdShow);

protected:
	void OnInit() override;

private:
	void InitResource(const CDuiString& resourcePath = _T("default"));
	void TrimProcessMemory();

	void LoadPlugins();
	void LoadMainWin();

private:
	MainWindow* m_pMainWnd = nullptr;


	int m_nCmdShow;
};
} // namespace DolPP

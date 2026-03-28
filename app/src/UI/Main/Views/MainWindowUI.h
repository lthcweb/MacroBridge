#pragma once
#include <UIlib.h>
#include "../Core/UIPageBase.h"

using namespace DuiLib;
namespace AIR
{
	class IScriptPlugin;
}

namespace DolPP
{

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//

	// class MainPanelUI;
	class NavBarUI;

	class MainWindowUI : public UIPageBase
	{
	public:
		MainWindowUI(CPaintManagerUI *mgr, CControlUI *pRoot);
		MainWindowUI(CPaintManagerUI *mgr, LPCTSTR pRootName = _T("main_root_container"));
		virtual ~MainWindowUI();

	protected:
		void OnInit() override;
		void OnBind() override;

	private:
		void FindAllControls();
		void AddBindingRules();

		void OnBtnConvert();
		void OnBtnClear();
		void OnSelectSourceType(TNotifyUI* param);
		void OnSelectTargetType(TNotifyUI* param);

		void AddPluginsToCombo(CComboUI* cb);
		void HandlePluginSelection(TNotifyUI* param, std::function<void(AIR::IScriptPlugin*)> action);

	private:
		CComboUI* m_pSourceCombo = nullptr;
		CComboUI* m_pTargetCombo = nullptr;
		CRichEditUI* m_pSourceEdit = nullptr;
		CRichEditUI* m_pTargetEdit = nullptr;
		CButtonUI* m_pConvertBtn = nullptr;
		CButtonUI* m_pClearBtn = nullptr;
	};
}

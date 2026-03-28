#pragma once
#include "MainWindowUI.h"
#include "Common/UIBinding/RuleBuilder.h"
#include "Common/Utils/string_conv.h"
#include "../../../Manager/PluginManager.h"

using namespace DuiLib;

namespace DolPP
{
	MainWindowUI::MainWindowUI(CPaintManagerUI *mgr, CControlUI *root)
		: UIPageBase(mgr, root)
	{
	}
	MainWindowUI::MainWindowUI(CPaintManagerUI *pManager, LPCTSTR pRootName)
		: UIPageBase(pManager, pRootName)
	{
	}

	MainWindowUI::~MainWindowUI()
	{
	}

	void MainWindowUI::OnInit()
	{
		AddPluginsToCombo(m_pSourceCombo);
		AddPluginsToCombo(m_pTargetCombo);
	}

	void MainWindowUI::OnBind()
	{
		FindAllControls();
		AddBindingRules();
	}

	void MainWindowUI::FindAllControls()
	{
		m_pSourceCombo = FIND_SUBCTRL(CComboUI, "combo_source_type");
		m_pTargetCombo = FIND_SUBCTRL(CComboUI, "combo_target_type");
		m_pSourceEdit = FIND_SUBCTRL(CRichEditUI, "edit_source_code");
		m_pTargetEdit = FIND_SUBCTRL(CRichEditUI, "edit_target_code");
		m_pConvertBtn = FIND_SUBCTRL(CButtonUI, "btn_convert");
		m_pClearBtn = FIND_SUBCTRL(CButtonUI, "btn_clear");
	}

	void MainWindowUI::AddBindingRules()
	{
		BindingRules().AddRule()
			.Name(_T("main_title_btn_maximize"))
			.Flags(MatchFlag::Name)
			.OnClick([this]() {
			::SendMessage(this->GetManager()->GetPaintWindow(), WM_SYSCOMMAND, SC_MAXIMIZE, 0);
				});

		BindingRules().AddRule()
			.Name(_T("main_title_btn_minimize"))
			.Flags(MatchFlag::Name)
			.OnClick([this]() {
			::SendMessage(this->GetManager()->GetPaintWindow(), WM_SYSCOMMAND, SC_MINIMIZE, 0);
				});

		BindingRules().AddRule()
			.Name(_T("main_title_btn_restore"))
			.Flags(MatchFlag::Name)
			.OnClick([this]() {
			::SendMessage(this->GetManager()->GetPaintWindow(), WM_SYSCOMMAND, SC_RESTORE, 0);
				});

		BindingRules().AddRule()
			.Name(_T("main_title_btn_close"))
			.Flags(MatchFlag::Name)
			.OnClick([this]() {
			::PostMessage(this->GetManager()->GetPaintWindow(), WM_CLOSE, 0, 0);
				});

		BindingRules().AddRule()
			.Control(m_pConvertBtn)
			.OnClick(this, &MainWindowUI::OnBtnConvert);

		BindingRules().AddRule()
			.Control(m_pClearBtn)
			.OnClick(this, &MainWindowUI::OnBtnClear);

		BindingRules().AddRule()
			.Control(m_pSourceCombo)
			.OnItemSelect(this, &MainWindowUI::OnSelectSourceType);

		BindingRules().AddRule()
			.Control(m_pTargetCombo)
			.OnItemSelect(this, &MainWindowUI::OnSelectTargetType);

	}

	void MainWindowUI::OnBtnConvert()
	{
		if (!m_pSourceEdit || !m_pTargetEdit)
			return;

		auto* sourcePlugin = PluginManager::GetInstance().GetSourcePlugin();
		auto* targetPlugin = PluginManager::GetInstance().GetTargetPlugin();
		if (sourcePlugin && targetPlugin) {
			std::vector<AIR::AIRDiagnostic> diags;
			std::string sourceCode = Utils::ToString(m_pSourceEdit->GetText().GetData());

			// Parse：AHK 文本 → AIR 树
			auto airRoot = sourcePlugin->Parse(sourceCode, diags);
			if (airRoot) {
				diags.clear();
				std::string output = targetPlugin->Generate(*airRoot, diags);
				m_pTargetEdit->SetText(Utils::utf8_to_utf16(output).c_str());

			}
		}
	}

	void MainWindowUI::OnBtnClear()
	{
		if (!m_pClearBtn)
			return;
		m_pTargetEdit->AppendText(_T("清理完成！\r\n"));

	}

	void MainWindowUI::OnSelectSourceType(TNotifyUI* param)
	{
		HandlePluginSelection(param, [this](AIR::IScriptPlugin* plugin) {
			PluginManager::GetInstance().SetSourcePlugin(plugin);
			if (m_pSourceEdit) {
			m_pSourceEdit->SetText(Utils::utf8_to_utf16(plugin->GetDemoCode()).c_str());
		}
		});

	}

	void MainWindowUI::OnSelectTargetType(TNotifyUI* param)
	{
		HandlePluginSelection(param, [](AIR::IScriptPlugin* plugin) {
			PluginManager::GetInstance().SetTargetPlugin(plugin);
			});
	}

	void MainWindowUI::AddPluginsToCombo(CComboUI* cb)
	{
		if (cb) {
			cb->RemoveAll();
			for (const auto& plugin : PluginManager::GetInstance().plugins()) {
				CListLabelElementUI* pNewItem = new CListLabelElementUI();
				std::string s = plugin.name + " (" + plugin.extension + ")";
				pNewItem->SetText(Utils::ToWString(s).c_str());
				pNewItem->SetUserData(Utils::ToWString(plugin.name).c_str());
				cb->Add(pNewItem);
			}
			if (cb->GetCount() > 0)
				cb->SelectItem(0);
		}
	}

	void MainWindowUI::HandlePluginSelection(TNotifyUI* param, std::function<void(AIR::IScriptPlugin*)> action)
	{
		if (! action) {
			return;
		}

		TNotifyUI* msg = reinterpret_cast<TNotifyUI*>(param);
		if (!msg) return;

		CComboUI* pCombo = static_cast<CComboUI*>(msg->pSender);
		if (!pCombo) return;
		auto index = pCombo->GetCurSel();
		if (index < 0 || index >= pCombo->GetCount()) {
			return;
		}
		auto item = pCombo->GetItemAt(index);
		if (item == nullptr) {
			return;
		}
		auto data = item->GetUserData().GetData();
		std::string macroName = Utils::ToString(data);

		auto& plugins = PluginManager::GetInstance().plugins();
		auto* plugin = PluginManager::GetInstance().findByName(macroName);

		if (!plugin) {
			return;
		}

		action(plugin);
	}

}

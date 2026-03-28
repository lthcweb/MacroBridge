#include "UIPageBase.h"

namespace DolPP
{

UIPageBase::UIPageBase(CPaintManagerUI* mgr, CControlUI* root)
	: m_pManager(mgr)
	, m_pRoot(root)
{
	m_szRootName = (m_pRoot ? m_pRoot->GetName().GetData() : _T(""));
	// 不调用虚函数，不进行绑定或初始化
}

UIPageBase::UIPageBase(CPaintManagerUI* mgr, LPCTSTR rootName)
	: m_pManager(mgr)
	, m_szRootName(rootName)
{
	if (m_pManager) {
		m_pRoot = static_cast<CControlUI*>(m_pManager->FindControl(rootName));
		if (!m_pRoot)   // rootName 不存在则置空
			m_szRootName = _T("");
	}
}

UIPageBase::~UIPageBase()
{
	Uninitialize();
}

void UIPageBase::Initialize()
{
	// 幂等检测
	bool expected = false;
	if (!m_initialized.compare_exchange_strong(expected, true))
		return; // 已初始化

	// 1) 派生类构建规则
	OnBind();

	// 2) 应用绑定
	m_bindingRules.ApplyRules(m_pRoot);

	// 3) 页面初始化逻辑
	OnInit();


}

void UIPageBase::Uninitialize()
{
	// 若没有初始化过，不执行 UnapplyRules
	bool expected = true;
	if (!m_initialized.compare_exchange_strong(expected, false)) {
		return;
	}

	// 解除绑定（保证控件不再调用已释放的回调）
	m_bindingRules.UnapplyRules(m_pRoot);

	m_bindingRules.ClearDelegates();
}

} // namespace DolPP


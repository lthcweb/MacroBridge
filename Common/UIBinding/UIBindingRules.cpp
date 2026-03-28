// UIBindingRules.cpp
#include <functional>
#include "RuleBuilder.h"
#include "UIBindingRules.h"

namespace DolPP
{

/**
 * @brief 析构函数
 *
 * 在 Debug 模式下检查是否仍存在未解绑的 delegate。
 *
 * 该检查用于提示使用者：
 *  - 是否遗漏了 UnapplyRules()
 *  - 是否在 UI 控件生命周期结束前销毁了 UIBindingRules
 *
 * 注意：
 *  - 这里只做调试提示
 *  - 不会尝试自动解绑或释放资源
 */
UIBindingRules::~UIBindingRules()
{
#ifdef _DEBUG
	if (!m_boundEntries.empty()) {
		OutputDebugString(
			_T("[UIBindingRules] Warning: UIBindingRules destroyed but some delegate still bound!\n")
		);
	}
#endif
}

/**
 * @brief DSL 构建入口
 *
 * 返回一个 RuleBuilder，用于链式构建 NotifyRule。
 * RuleBuilder 在析构时会自动 Commit。
 */
RuleBuilder UIBindingRules::AddRule() noexcept
{
	return RuleBuilder(*this);
}

/**
 * @brief 直接添加规则描述（非 DSL 路径）
 *
 * 该接口仅用于插入 NotifyRule 数据，
 * 不涉及 delegate 生命周期管理。
 *
 * 通常用于：
 *  - 测试
 *  - 反序列化
 *  - 其它系统直接注入规则
 */
void UIBindingRules::AddRule(const NotifyRule& r) noexcept
{
	std::lock_guard<std::mutex> lk(m_mutex);
	m_rules.push_back(r);
}

/**
 * @brief RuleBuilder 专用的内部规则提交接口
 *
 * 与 AddRule 的区别在于：
 *  - 该接口仅由 RuleBuilder::Commit() 调用
 *  - 规则中的 delegate 生命周期已通过 TakeOwnership() 单独处理
 */
void UIBindingRules::AddRuleInternal(const NotifyRule& r) noexcept
{
	std::lock_guard<std::mutex> lk(m_mutex);
	m_rules.push_back(r);
}

/**
 * @brief 判断指定控件是否已绑定过某个 delegate
 *
 * 用于保证 ApplyRules 的幂等性：
 *  - 同一个 (Control, Delegate) 组合只会绑定一次
 *
 * @param ctrl 目标控件
 * @param cb   delegate 的弱引用
 */
bool UIBindingRules::IsBound(CControlUI* ctrl, TNotifyCallback cb) const noexcept
{
	if (!ctrl)
		return false;

	auto cbSp = cb.lock();
	if (!cbSp)
		return false;

	for (auto& p : m_boundEntries) {
		if (p.first != ctrl)
			continue;

		if (auto pSp = p.second.lock()) {
			if (pSp.get() == cbSp.get())
				return true;
		}
	}

	return false;
}

/**
 * @brief 记录一次成功的绑定关系
 *
 * 该记录用于：
 *  - 防止重复绑定
 *  - 后续精确解绑
 *
 * 该函数不与 DuiLib 交互，仅维护内部状态。
 */
void UIBindingRules::RecordBinding(CControlUI* ctrl, TNotifyCallback cb) noexcept
{
	if (ctrl && cb.lock())
		m_boundEntries.emplace_back(ctrl, std::move(cb));
}

/**
 * @brief 移除一条绑定记录
 *
 * 当解绑或 delegate 失效时调用。
 * 若 weak_ptr 已过期，会顺便清理对应记录。
 */
void UIBindingRules::RemoveBindingRecord(CControlUI* ctrl, TNotifyCallback cb) noexcept
{
	if (!ctrl)
		return;

	auto cbSp = cb.lock();
	if (!cbSp)
		return;

	m_boundEntries.erase(
		std::remove_if(
			m_boundEntries.begin(),
			m_boundEntries.end(),
			[&](auto& p) {
				if (p.first != ctrl)
					return false;

				auto pSp = p.second.lock();

				// weak_ptr 已失效，直接移除记录
				if (!pSp)
					return true;

				// 判断是否为同一个 delegate
				return pSp.get() == cbSp.get();
			}),
		m_boundEntries.end());
}

/**
 * @brief 接收 delegate 生命周期的唯一所有权
 *
 * 该接口由 RuleBuilder 在 Commit 阶段调用。
 * UIBindingRules 成为 delegate 的唯一 owner。
 *
 * 注意：
 *  - DuiLib 在绑定时会 Copy delegate
 *  - Copy 出来的副本生命周期由 DuiLib 管理
 *  - 这里保存的是“原始 delegate 本体”
 */
void UIBindingRules::TakeOwnership(std::shared_ptr<CDelegateBase> cb) noexcept
{
	if (!cb)
		return;

	std::lock_guard<std::mutex> lk(m_mutex);
	m_callbackLifetime.emplace_back(std::move(cb));
}

/**
 * @brief 判断单个控件是否满足某条规则
 *
 * 该函数仅执行匹配判断，不产生任何副作用。
 * 实际绑定行为由 ApplyRules() 控制。
 */
bool UIBindingRules::MatchSingle(CControlUI* ctrl, const NotifyRule& r) const noexcept
{
	if (!ctrl)
		return false;

	bool mName = false, mClass = false, mGroup = false, mTag = false;
	bool mUD = false, mVis = false, mEn = false, mCont = false;

	if (HasFlag(r.flags, MatchFlag::Name) && r.name)
		mName = (_tcscmp(ctrl->GetName(), r.name) == 0);

	if (HasFlag(r.flags, MatchFlag::Class) && r.clazz)
		mClass = (_tcscmp(ctrl->GetClass(), r.clazz) == 0);

	if (HasFlag(r.flags, MatchFlag::Group) && r.group) {
		auto opt = dynamic_cast<COptionUI*>(ctrl);
		if (opt)
			mGroup = (_tcscmp(opt->GetGroup(), r.group) == 0);
	}

	if (HasFlag(r.flags, MatchFlag::Tag) && r.tag) {
		CDuiString ud = ctrl->GetUserData();
		mTag = (ud.Find(r.tag) >= 0);
	}

	if (HasFlag(r.flags, MatchFlag::UserDataNotEmpty))
		mUD = !ctrl->GetUserData().IsEmpty();

	if (HasFlag(r.flags, MatchFlag::Visible))
		mVis = ctrl->IsVisible();

	if (HasFlag(r.flags, MatchFlag::Enabled))
		mEn = ctrl->IsEnabled();

	if (HasFlag(r.flags, MatchFlag::Container))
		mCont = (ctrl->GetInterface(_T("Container")) != nullptr);

	// ALL：必须满足所有启用的条件
	if (r.behavior == MatchBehavior::All) {
		if (HasFlag(r.flags, MatchFlag::Name) && !mName) return false;
		if (HasFlag(r.flags, MatchFlag::Class) && !mClass) return false;
		if (HasFlag(r.flags, MatchFlag::Group) && !mGroup) return false;
		if (HasFlag(r.flags, MatchFlag::Tag) && !mTag) return false;
		if (HasFlag(r.flags, MatchFlag::UserDataNotEmpty) && !mUD) return false;
		if (HasFlag(r.flags, MatchFlag::Visible) && !mVis) return false;
		if (HasFlag(r.flags, MatchFlag::Enabled) && !mEn) return false;
		if (HasFlag(r.flags, MatchFlag::Container) && !mCont) return false;
		return true;
	}

	// ANY：满足任意一个条件即可
	return
		(HasFlag(r.flags, MatchFlag::Name) && mName) ||
		(HasFlag(r.flags, MatchFlag::Class) && mClass) ||
		(HasFlag(r.flags, MatchFlag::Group) && mGroup) ||
		(HasFlag(r.flags, MatchFlag::Tag) && mTag) ||
		(HasFlag(r.flags, MatchFlag::UserDataNotEmpty) && mUD) ||
		(HasFlag(r.flags, MatchFlag::Visible) && mVis) ||
		(HasFlag(r.flags, MatchFlag::Enabled) && mEn) ||
		(HasFlag(r.flags, MatchFlag::Container) && mCont);
}

/**
 * @brief 将所有规则应用到指定控件树
 *
 * 执行流程：
 *  1. 先处理 NotifyRule::control 直接绑定的规则
 *  2. 对控件树进行 DFS 遍历
 *  3. 对每个控件尝试匹配规则并绑定 delegate
 *
 * 设计保证：
 *  - 幂等：不会重复绑定同一 delegate
 *  - delegate 必须仍然存活（weak_ptr lock 成功）
 */
void UIBindingRules::ApplyRules(CControlUI* root) noexcept
{
	if (!root)
		return;

	// 1. 快速复制规则列表（持锁时间短）
	std::vector<NotifyRule> rulesCopy;
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		rulesCopy = m_rules;
	}

	// ① 直接绑定到指定控件的规则
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		for (auto& r : rulesCopy) {
			if (!r.control)
				continue;

			auto cb = r.callback.lock();
			if (!cb)
				continue;

			if (!IsBound(r.control, r.callback)) {
				r.control->OnNotify += *cb;
				RecordBinding(r.control, r.callback);
			}
		}
	}
		// ② DFS 遍历控件树并按规则匹配
	std::function<void(CControlUI*)> dfs = [&](CControlUI* ctrl) {
		for (auto& r : rulesCopy) {
			if (r.control)
				continue;

			if (!MatchSingle(ctrl, r))
				continue;

			if (auto cb = r.callback.lock()) {
				std::lock_guard<std::mutex> lk(m_mutex);
				if (!IsBound(ctrl, r.callback)) {
					ctrl->OnNotify += *cb;
					RecordBinding(ctrl, r.callback);
				}
			}
		}

		auto container =
			static_cast<CContainerUI*>(ctrl->GetInterface(_T("Container")));
		if (!container)
			return;

		for (int i = 0; i < container->GetCount(); i++)
			dfs(container->GetItemAt(i));
	};

	dfs(root);
}

/**
 * @brief 从指定控件树解绑所有由本系统绑定的 delegate
 *
 * 该操作仅影响：
 *  - 通过 UIBindingRules 绑定的 delegate
 *
 * 不会影响控件上可能存在的其它事件处理逻辑。
 */
void UIBindingRules::UnapplyRules(CControlUI* root) noexcept
{
	if (!root)
		return;

	std::lock_guard<std::mutex> lk(m_mutex);

	std::function<void(CControlUI*)> dfs = [&](CControlUI* ctrl) {
		auto it = m_boundEntries.begin();
		while (it != m_boundEntries.end()) {
			if (it->first == ctrl) {
				TNotifyCallback weakCb = it->second;
				if (auto cb = weakCb.lock())
					ctrl->OnNotify -= *cb;

				it = m_boundEntries.erase(it);
			}
			else {
				++it;
			}
		}

		auto container =
			static_cast<CContainerUI*>(ctrl->GetInterface(_T("Container")));
		if (!container)
			return;

		for (int i = 0; i < container->GetCount(); i++)
			dfs(container->GetItemAt(i));
	};

	dfs(root);
}

/**
 * @brief 清空所有规则描述
 *
 * 注意：
 *  - 不会自动解绑 delegate
 *  - 不会释放 delegate 生命周期
 *
 * 通常应在 UnapplyRules() 之后调用。
 */
void UIBindingRules::ClearRules() noexcept
{
	std::lock_guard<std::mutex> lk(m_mutex);
	m_rules.clear();
	m_boundEntries.clear();
}

/**
 * @brief 释放并清理所有 delegate
 *
 * 使用前提：
 *  - 已确保 delegate 不再被 DuiLib 持有
 *    （通常意味着已调用 UnapplyRules()）
 *
 * 执行步骤：
 *  1. 从控件上 -= 所有仍绑定的 delegate
 *  2. 清空规则中的 weak_ptr
 *  3. 释放 shared_ptr（唯一 owner）
 */
void UIBindingRules::ClearDelegates() noexcept
{
	std::vector<std::shared_ptr<CDelegateBase>> delegates;
	{
		std::lock_guard<std::mutex> lk(m_mutex);

		// 1. 主动从控件上解绑所有仍存活的 delegate
		for (auto& entry : m_boundEntries) {
			if (auto cb = entry.second.lock()) {
				if (entry.first)
					entry.first->OnNotify -= *cb;
			}
		}
		m_boundEntries.clear();

		// 2. 清空规则中的弱引用
		for (auto& r : m_rules) {
			r.callback.reset();
		}

		// 3. 转移 shared_ptr 到局部变量
		delegates = std::move(m_callbackLifetime);
		m_callbackLifetime.clear();
	}
	// ✅ 锁外释放，确保在解绑完成后才销毁对象
	delegates.clear();
}

} // namespace DolPP

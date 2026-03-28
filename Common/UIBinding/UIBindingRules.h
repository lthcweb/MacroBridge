// UIBindingRules.h
#pragma once
#include <UIlib.h>
#include <vector>
#include <mutex>
#include "NotifyRule.h"
#include "IRuleSink.h"

namespace DolPP
{

using namespace DuiLib;

class RuleBuilder;

/**
 * @brief UI 事件绑定系统的核心管理类。
 *
 * UIBindingRules 是整个 UIBind 框架的“执行引擎”，
 * 负责将 NotifyRule 描述的规则真正应用到 DuiLib 的控件树上。
 *
 * ------------------------------------------------------------------
 * 【核心职责】
 * ------------------------------------------------------------------
 *  1. 保存所有由 DSL 构建的 NotifyRule
 *  2. 在指定控件树上应用规则（ApplyRules）
 *  3. 精确解绑此前由本系统绑定的回调（UnapplyRules）
 *  4. 统一托管所有 delegate 的生命周期（shared_ptr）
 *  5. 作为 IRuleSink，承接 RuleBuilder 的构建结果
 *
 * ------------------------------------------------------------------
 * 【设计特性】
 * ------------------------------------------------------------------
 *  - 幂等性：
 *      同一 (Control, Delegate) 组合只会被绑定一次
 *
 *  - 生命周期安全：
 *      delegate 的唯一所有权集中在本类中管理，
 *      与 DuiLib 内部 Copy 出来的对象严格隔离
 *
 *  - 扩展友好：
 *      同时支持：
 *        - Control() 直接绑定
 *        - 规则匹配 + DFS 自动绑定
 *
 * ------------------------------------------------------------------
 * 【线程模型说明】
 * ------------------------------------------------------------------
 *  - 所有公开接口均为“线程安全的最小保障”
 *  - 内部通过 std::mutex 保护规则与绑定记录
 *  - 实际的 OnNotify 绑定 / 解绑应在 UI 线程执行
 */
class UIBindingRules : public IRuleSink
{
public:
	UIBindingRules() = default;

	/**
	 * @brief 析构函数
	 *
	 * Debug 模式下会检测是否仍存在未解绑的 delegate，
	 * 用于提示调用者可能遗漏 UnapplyRules()。
	 */
	~UIBindingRules();

	/**
	 * @brief DSL 构建入口。
	 *
	 * 返回一个 RuleBuilder 对象，用于链式构建 NotifyRule。
	 *
	 * 使用示例：
	 * @code
	 * rules.AddRule()
	 *     .Name(_T("btn_ok"))
	 *     .OnClick(this, &Page::OnOk);
	 * @endcode
	 */
	RuleBuilder AddRule() noexcept;

	/**
	 * @brief 直接添加一条规则描述。
	 *
	 * 该接口通常用于：
	 *  - 反序列化规则
	 *  - 其它系统直接注入规则
	 *
	 * 不涉及 delegate 生命周期管理。
	 */
	void AddRule(const NotifyRule& r) noexcept;

	/**
	 * @brief 在指定控件树上应用所有规则。
	 *
	 * 行为说明：
	 *  - 从 root 开始进行 DFS 遍历
	 *  - 对每个控件检查是否匹配规则
	 *  - 对匹配的控件绑定对应的 delegate
	 *
	 * 设计保证：
	 *  - 幂等：同一 (ctrl, callback) 不会重复绑定
	 *  - 若 NotifyRule::control 不为空，则直接绑定，
	 *    不参与控件树遍历和匹配
	 *
	 * @param root 控件树根节点
	 */
	void ApplyRules(CControlUI* root) noexcept;

	/**
	 * @brief 从指定控件树上解绑所有由本系统绑定的事件。
	 *
	 * 仅移除通过 UIBindingRules 绑定的 delegate，
	 * 不影响控件上可能存在的其它事件处理逻辑。
	 *
	 * @param root 控件树根节点
	 */
	void UnapplyRules(CControlUI* root) noexcept;

	/**
	 * @brief 清除所有规则描述。
	 *
	 * 注意：
	 *  - 该操作不会自动解绑已绑定的 delegate
	 *  - 通常应在 UnapplyRules() 之后调用
	 */
	void ClearRules() noexcept;

	/**
	 * @brief 释放所有 delegate 对象。
	 *
	 * 使用前提：
	 *  - 已确保这些 delegate 不再被 DuiLib 持有
	 *    （即已调用 UnapplyRules()）
	 *
	 * 该函数将：
	 *  - 主动从控件上 -= delegate
	 *  - 清空规则中的 weak_ptr
	 *  - 释放所有 shared_ptr（唯一 owner）
	 */
	void ClearDelegates() noexcept;

	/**
	 * @brief 获取当前规则数量。
	 *
	 * 线程安全，仅用于调试 / 统计。
	 */
	size_t GetRuleCount() const noexcept
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		return m_rules.size();
	}

	/**
	 * @brief 接收 delegate 生命周期所有权。
	 *
	 * 该接口由 RuleBuilder 在 Commit 阶段调用，
	 * 用于将新创建的 delegate 的唯一所有权
	 * 交由 UIBindingRules 统一管理。
	 */
	void TakeOwnership(std::shared_ptr<CDelegateBase> cb) noexcept override;

private:
	/**
	 * @brief RuleBuilder 专用的内部 AddRule 接口。
	 *
	 * 用于将构建完成的 NotifyRule 插入规则列表，
	 * 不涉及 delegate 生命周期。
	 */
	void AddRuleInternal(const NotifyRule& r) noexcept override;

	/**
	 * @brief 判断单个控件是否满足某条规则。
	 *
	 * 该函数仅执行匹配判断，不进行任何绑定操作。
	 */
	bool MatchSingle(CControlUI* ctrl, const NotifyRule& r) const noexcept;

	/**
	 * @brief 检查指定控件是否已经绑定过某个回调。
	 *
	 * 用于保证 ApplyRules 的幂等性。
	 */
	bool IsBound(CControlUI* ctrl, TNotifyCallback cb) const noexcept;

	/**
	 * @brief 记录一次成功的绑定关系。
	 *
	 * 用于：
	 *  - 防止重复绑定
	 *  - 后续精确解绑
	 */
	void RecordBinding(CControlUI* ctrl, TNotifyCallback cb) noexcept;

	/**
	 * @brief 移除一条绑定记录。
	 *
	 * 通常在解绑或 delegate 失效时调用。
	 */
	void RemoveBindingRecord(CControlUI* ctrl, TNotifyCallback cb) noexcept;

private:
	mutable std::mutex m_mutex; ///< 保护规则、delegate 及绑定记录

	std::vector<NotifyRule> m_rules; ///< 所有规则描述
	std::vector<std::shared_ptr<CDelegateBase>>
		m_callbackLifetime; ///< delegate 生命周期唯一托管者

	std::vector<std::pair<CControlUI*, TNotifyCallback>>
		m_boundEntries; ///< 已绑定的 (控件, delegate) 记录（非 owning）
};

} // namespace DolPP

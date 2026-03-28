// RuleBuilder.cpp
#include "RuleBuilder.h"

namespace DolPP
{

/**
 * @brief 构造函数
 *
 * RuleBuilder 并不拥有 IRuleSink，
 * 仅保存其引用，用于在 Commit 阶段提交规则。
 *
 * @param sink 规则接收端（通常为 UIBindingRules）
 */
RuleBuilder::RuleBuilder(IRuleSink& sink)
	: m_sink(sink)
{
}

/**
 * @brief 析构函数（RAII 自动提交点）
 *
 * 若用户在 DSL 构建过程中未显式调用 Commit()，
 * 则在 RuleBuilder 离开作用域时自动提交规则。
 *
 * 设计目的：
 *  - 防止因遗漏 Commit() 导致规则失效
 *  - 保证 DSL 语法的“声明即生效”直觉
 *
 * 注意：
 *  - 若已手动 Commit，则析构时不会重复提交
 */
RuleBuilder::~RuleBuilder()
{
	if (!m_committed)
		Commit();
}

/**
 * @brief 提交当前构建完成的规则。
 *
 * Commit 是 RuleBuilder 的“最终阶段”，
 * 负责将 DSL 构建得到的 NotifyRule 转换为
 * UIBindingRules 可执行的内部状态。
 *
 * ------------------------------------------------------------------
 * 【执行流程】
 * ------------------------------------------------------------------
 *  1. 若存在 delegateFactory，则构造实际 delegate 对象
 *  2. NotifyRule 中仅保存 weak_ptr（非 owning）
 *  3. 通过 IRuleSink::TakeOwnership() 明确移交生命周期
 *  4. 将 NotifyRule 数据提交给规则执行端
 *
 * ------------------------------------------------------------------
 * 【生命周期说明】
 * ------------------------------------------------------------------
 *  - delegate 的唯一 owner 为 UIBindingRules
 *  - RuleBuilder / NotifyRule 不参与对象释放
 *  - DuiLib 在绑定时会 Copy delegate，
 *    副本的生命周期由 DuiLib 自身管理
 */
void RuleBuilder::Commit()
{
	// 防止重复提交
	if (m_committed)
		return;

	// 若存在延迟构造工厂，则创建实际 delegate
	if (m_rule.delegateFactory) {
		// 1. 构造 delegate（shared_ptr，本体）
		auto sp = m_rule.delegateFactory(m_rule.userFilter);

		// 2. NotifyRule 仅保存弱引用，用于判重与解绑
		m_rule.callback = sp;

		// 3. 明确将 delegate 的唯一所有权交给规则执行端
		m_sink.TakeOwnership(sp);
	}

	// 4. 提交规则描述（不涉及 delegate 生命周期）
	m_sink.AddRuleInternal(m_rule);

	m_committed = true;
}

} // namespace DolPP

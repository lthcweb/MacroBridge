// IRuleSink.h
#pragma once
#include "NotifyRule.h"

namespace DolPP
{

/**
 * @brief 规则接收接口（Rule Sink）。
 *
 * IRuleSink 是 RuleBuilder 向外提交构建结果的唯一出口，
 * 用于 **解耦 DSL 构建过程** 与 **规则的存储 / 执行 / 生命周期管理**。
 *
 * ------------------------------------------------------------------
 * 【为什么需要 IRuleSink】
 * ------------------------------------------------------------------
 * RuleBuilder 的职责仅限于：
 *  - 解析 DSL 调用顺序
 *  - 组合匹配条件、事件类型、过滤器
 *  - 构建 NotifyRule 这一“纯数据描述”
 *
 * 它不应该、也不能：
 *  - 知道规则最终存放在哪里
 *  - 管理 delegate 的生命周期
 *  - 参与 UI 控件树遍历或事件绑定
 *
 * 因此通过 IRuleSink：
 *  - RuleBuilder 与具体实现（如 UIBindingRules）完全解耦
 *  - 后续可以替换 / 扩展不同的规则执行后端
 *
 * ------------------------------------------------------------------
 * 【典型实现】
 * ------------------------------------------------------------------
 * - UIBindingRules：将规则应用到 DuiLib 控件树
 *
 * 理论上，也可以实现：
 * - DebugRuleSink（仅记录规则，用于调试）
 * - ScriptRuleSink（转发到脚本系统）
 * - MockRuleSink（用于单元测试）
 */
struct IRuleSink
{
	/**
	 * @brief 虚析构函数
	 *
	 * 确保通过 IRuleSink 指针删除派生类对象时，
	 * 能正确调用派生类析构函数。
	 */
	virtual ~IRuleSink() = default;

	/**
	 * @brief 接收一条已构建完成的 NotifyRule（内部接口）。
	 *
	 * 该接口由 RuleBuilder 在 Commit 阶段调用，
	 * 用于将“规则描述数据”提交给具体的绑定系统。
	 *
	 * 设计约束：
	 *  - NotifyRule 是只读数据结构
	 *  - IRuleSink 不应修改其内容
	 *
	 * @param rule 已构建完成的规则描述
	 */
	virtual void AddRuleInternal(const NotifyRule& rule) = 0;

	/**
	 * @brief delegate 生命周期所有权交接接口。
	 *
	 * RuleBuilder 在 Commit 阶段会创建实际的事件代理对象
	 *（CDelegateBase 派生类，通常为 UniversalDelegate），
	 * 并通过该接口将 **唯一所有权** 明确移交给规则执行端。
	 *
	 * 设计说明：
	 *  - 传入的是 shared_ptr，表示“唯一 owner”
	 *  - RuleBuilder / NotifyRule 自身只保留 weak_ptr
	 *  - 具体实现（如 UIBindingRules）负责：
	 *      - 保存 shared_ptr
	 *      - 确保 delegate 生命周期覆盖其绑定周期
	 *
	 * @param cb 已构建完成的事件代理对象
	 */
	virtual void TakeOwnership(std::shared_ptr<CDelegateBase> cb) = 0;
};

} // namespace DolPP

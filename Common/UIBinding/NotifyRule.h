// NotifyRule.h
#pragma once
#include <UIlib.h>
#include <functional>
#include <memory>
//#include <type_traits>

namespace DolPP
{

using namespace DuiLib;

/**
 * @brief 事件绑定系统中使用的回调句柄类型。
 *
 * TNotifyCallback 表示一个“已构建完成的事件代理（delegate）”。
 *
 * 设计说明：
 *  - 实际对象类型为 CDelegateBase 或其派生类
 *  - 生命周期由 UIBindingRules 统一以 shared_ptr 托管
 *  - NotifyRule / 绑定记录中只保存 weak_ptr，用于：
 *      - 判重（避免重复绑定）
 *      - 精确解绑
 *
 * 该类型本身不拥有对象，不负责释放。
 */
//using TNotifyCallback = CDelegateBase*;
using TNotifyCallback = std::weak_ptr<CDelegateBase>;

/**
 * @brief 委托延迟构造工厂类型（DelegateFactory）
 *
 * 该工厂在 DSL 构建阶段被创建并保存，
 * 在 RuleBuilder::Commit() 阶段才会真正执行。
 *
 * 设计目的：
 *  - 消除 Filter / OnXXX 调用顺序依赖
 *  - 允许在 Commit 时注入最终确定的 UIEventFilter
 *
 * 参数：
 *  - std::function<bool(TNotifyUI*)>：最终合并后的过滤器
 *
 * 返回：
 *  - 构造完成、可直接交给 DuiLib 使用的 delegate（shared_ptr）
 */
using DelegateFactory =
std::function<std::shared_ptr<CDelegateBase>(
	std::function<bool(TNotifyUI*)>
	)>;


/**
 * @brief 控件匹配标志位（可组合的位掩码）。
 *
 * 用于描述 NotifyRule 在控件树遍历时，
 * 需要对哪些属性进行匹配判断。
 *
 * 该枚举仅描述“检查哪些条件”，
 * 实际匹配逻辑由 UIBindingRules 解释执行。
 */
enum class MatchFlag : uint32_t
{
	None = 0,        ///< 不启用任何匹配条件
	Name = 1u << 0,   ///< 匹配控件名称（GetName）
	Class = 1u << 1,   ///< 匹配控件类名（GetClass）
	Group = 1u << 2,   ///< 匹配分组（仅 Option / CheckBox 有效）
	Tag = 1u << 3,   ///< 匹配 UserData 中的 Tag
	UserDataNotEmpty = 1u << 4,   ///< 要求 UserData 非空
	Visible = 1u << 5,   ///< 要求控件可见
	Enabled = 1u << 6,   ///< 要求控件可用
	Container = 1u << 7,   ///< 要求控件实现 Container 接口
};

/**
 * @brief 组合两个 MatchFlag 标志位。
 *
 * 允许使用：
 *   MatchFlag::Name | MatchFlag::Visible
 */
inline MatchFlag operator|(MatchFlag a, MatchFlag b)
{
	return static_cast<MatchFlag>(
		static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
		);
}

/**
 * @brief 判断标志集合 a 中是否包含 b。
 */
inline bool HasFlag(MatchFlag a, MatchFlag b)
{
	return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

/**
 * @brief 匹配行为模式。
 *
 * 控制当启用了多个 MatchFlag 时，
 * 匹配条件之间的组合关系。
 */
enum class MatchBehavior
{
	All,    ///< 必须满足所有启用的匹配条件（默认）
	Any     ///< 满足任意一个条件即可
};


/**
 * @brief 事件绑定系统中的“规则描述结构”（纯数据）。
 *
 * NotifyRule 是整个 UIBind DSL 的核心数据载体：
 *  - 由 RuleBuilder 负责构建
 *  - 由 UIBindingRules 负责解释和执行
 *
 * ------------------------------------------------------------------
 * 【设计原则】
 * ------------------------------------------------------------------
 *  - 纯数据结构（POD 风格）
 *  - 不包含任何业务逻辑
 *  - 不包含虚函数 / 构造 / 析构
 *  - 所有字段均为 public，便于 DSL 直接赋值
 *
 * NotifyRule 本身并不会执行任何事件绑定，
 * 它只描述“在什么条件下，绑定什么事件，使用什么回调”。
 */
struct NotifyRule
{
	/**
	 * @brief 直接绑定到指定控件（跳过匹配和遍历）。
	 *
	 * 使用场景：
	 *  - 已持有控件指针
	 *  - 动态创建控件
	 *  - 不希望通过 Name / Class 等方式匹配
	 *
	 * 规则：
	 *  - 当 control != nullptr 时，
	 *    所有普通匹配字段（name / class / group / tag / flags）
	 *    都将被忽略。
	 */
	CControlUI* control = nullptr;

	// ----------- 普通匹配字段（仅在 control == nullptr 时生效） -----------

	LPCTSTR name = nullptr;  ///< 控件名称（GetName）
	LPCTSTR clazz = nullptr;  ///< 控件类名（GetClass）
	LPCTSTR group = nullptr;  ///< 控件分组名（Option / CheckBox）
	LPCTSTR tag = nullptr;  ///< UserData 中的 Tag 子串

	MatchFlag     flags = MatchFlag::None; ///< 启用的匹配条件集合
	MatchBehavior behavior = MatchBehavior::All; ///< 多条件组合方式

	// ----------- 事件绑定相关字段 -----------

	/**
	 * @brief 事件类型字符串（DuiLib 事件名）。
	 *
	 * 示例：
	 *  - "click"
	 *  - "selectchanged"
	 *
	 * 若为空字符串，则表示：
	 *  - 不对事件类型做额外过滤
	 *  - 由 delegate 自身决定是否处理该事件
	 */
	CDuiString eventType;

	/**
	 * @brief 用户自定义的前置过滤器。
	 *
	 * 返回 false 时：
	 *  - 事件被拦截
	 *  - 不会进入最终回调
	 */
	std::function<bool(TNotifyUI*)> userFilter;

	/**
	 * @brief 延迟构造 delegate 的工厂函数。
	 *
	 * 在 DSL 构建阶段：
	 *  - 仅保存构造逻辑（捕获 this / 函数指针 / 参数）
	 *
	 * 在 Commit 阶段：
	 *  - 结合最终确定的 userFilter
	 *  - 实际创建 CDelegateBase 派生对象
	 */
	DelegateFactory delegateFactory = nullptr;

	/**
	 * @brief 已构建完成的事件回调句柄。
	 *
	 * 特点：
	 *  - 由 RuleBuilder 在 Commit 时生成
	 *  - 由 UIBindingRules 统一托管生命周期
	 *  - NotifyRule 中仅保存 weak_ptr，不拥有对象
	 *
	 * 该字段用于：
	 *  - 防止重复绑定
	 *  - 精确解绑已绑定的 delegate
	 */
	TNotifyCallback callback;
};

} // namespace DolPP



	 // RuleBuilder.h
#pragma once
#include <tuple>
#include <utility>
#include <type_traits>
#include <functional>
#include "IRuleSink.h"
#include "EventDelegate.h"

namespace DolPP
{

/**
 * ============================================================================
 * DolPP :: UIBinding DSL 规则构建器（RuleBuilder）
 * ============================================================================
 *
 * 【定位说明】
 * ----------------------------------------------------------------------------
 * RuleBuilder 是 UIBinding 系统中的“声明式构建层（DSL Layer）”，
 * 负责将“用户意图”转换为 NotifyRule 这种**纯数据描述结构**。
 *
 * 它本身：
 *  - 不遍历控件树
 *  - 不绑定事件
 *  - 不管理 delegate 生命周期
 *
 * 所有执行行为都交由 IRuleSink（通常是 UIBindingRules）完成。
 *
 * ---------------------------------------------------------------------------
 * 【设计目标】
 * ---------------------------------------------------------------------------
 *  - 解耦 UI 布局（XML / 控件树）与业务代码
 *  - 用声明式、链式语法代替 OnNotify + if/switch
 *  - 在不修改 DuiLib 的前提下，提供接近 Qt / C# 的绑定体验
 *
 * ---------------------------------------------------------------------------
 * 【整体协作关系】
 * ---------------------------------------------------------------------------
 *
 *   RuleBuilder        NotifyRule        UniversalDelegate
 *       │                   │                   │
 *       │ 构建 DSL           │ 规则数据           │ 封装 std::function
 *       ▼                   ▼                   ▼
 *   IRuleSink  ───────▶  UIBindingRules  ───────▶  DuiLib EventSystem
 *
 * ---------------------------------------------------------------------------
 * 【关键设计点】
 * ---------------------------------------------------------------------------
 *  ✔ Fluent Interface（链式 DSL）
 *  ✔ 完美转发 + SFINAE 适配多种回调签名
 *  ✔ 支持成员函数 / Lambda / 全局函数
 *  ✔ 支持带参 / 无参回调（TNotifyUI* 可选）
 *  ✔ 支持 bool / void 返回值（void 自动映射为 true）
 *  ✔ delegateFactory 延迟构造，消除 Filter / OnXXX 顺序依赖
 *  ✔ RAII 自动提交（析构时 Commit）
 *
 * ---------------------------------------------------------------------------
 *  *
 * 【典型用法示例】
 * ----------------------------------------------------------------------------
 *
 * BindingRules rules;
 *
 * 1️⃣ 最基础：按控件名绑定点击事件（成员函数）
 *
 * rules.AddRule()
 *     .Name(_T("btn_login"))
 *     .OnClick(this, &LoginPage::OnLoginClicked);
 *
 * 2️⃣ Lambda 绑定（带事件参数）
 *
 * rules.AddRule()
 *     .Name(_T("btn_close"))
 *     .OnClick([this](TNotifyUI* msg) {
 *         CloseWindow();
 *         return true;
 *     });
 *
 * 3️⃣ 无参 Lambda / 无参成员函数（完全合法）
 *
 * rules.AddRule()
 *     .Name(_T("btn_refresh"))
 *     .OnClick([this]() {
 *         RefreshData();
 *     });
 *
 * 4️⃣ 前置过滤器（Filter）
 *
 * rules.AddRule()
 *     .Name(_T("btn_submit"))
 *     .Filter([this](auto*) {
 *         return !m_isSubmitting;   // false 将直接拦截事件
 *     })
 *     .OnClick(this, &Page::OnSubmit);
 *
 * 5️⃣ 指定具体控件（跳过 Name / Class 匹配）
 *
 * rules.AddRule()
 *     .Control(m_pLoginBtn)
 *     .OnClick(this, &LoginPage::OnLoginClicked);
 *
 * 6️⃣ 成员函数 + 预传参数（高级用法）
 *
 * rules.AddRule()
 *     .Name(_T("btn_tab1"))
 *     .OnClick(this, &Page::OnSwitchTab, 1);
 *

 * ============================================================================
 */


/**
 * @brief 内部通用调用分发器（签名抹平器）。
 *
 * EventInvoker 利用：
 *  - std::invoke
 *  - if constexpr
 *  - std::is_invocable
 *
 * 在编译期判断用户提供的回调函数“能不能被调用”，
 * 并自动选择正确的调用形式。
 *
 * 该结构仅用于 RuleBuilder 内部，不暴露给外部系统。
 */
struct EventInvoker
{
	/**
	 * @brief 统一回调分发入口
	 *
	 * @tparam F    可调用对象类型（函数指针 / Lambda / 成员函数等）
	 * @tparam Args 预传参数包
	 *
	 * @param f     可调用对象
	 * @param msg   DuiLib 原始事件指针
	 * @param args  预先捕获的附加参数
	 *
	 * @return bool 返回值，符合 DuiLib 的事件处理语义
	 */
	template<typename F, typename... Args>
	static bool Call(F&& f, TNotifyUI* msg, Args&&... args)
	{
		// 优先级 1：
		// 尝试调用形式：f(args..., msg)
		// 用于支持：
		//  - 成员函数 + 预传参数 + TNotifyUI*
		//  - 带参 Lambda
		if constexpr (std::is_invocable_v<F, Args..., TNotifyUI*>) {
			return InvokeWithBool(std::forward<F>(f),
				std::forward<Args>(args)...,
				msg);
		}
		// 优先级 2：
		// 尝试调用形式：f(args...)
		// 用于支持：
		//  - 无参成员函数
		//  - 无参 Lambda
		else if constexpr (std::is_invocable_v<F, Args...>) {
			return InvokeWithBool(std::forward<F>(f),
				std::forward<Args>(args)...);
		}
		else {
			// 函数签名完全不匹配，编译期直接报错
			static_assert(always_false_v<F>,
				"Error: Incompatible function signature");
			// 理论上不可达，仅用于防止编译器警告
			return true;
		}
	}

private:
	/**
	 * @brief 延迟触发的编译期 false
	 *
	 * 用于 static_assert 的模板依赖技巧，
	 * 避免在模板解析阶段直接报错。
	 */
	template<class>
	static constexpr bool always_false_v = false;

	/**
	 * @brief 返回值适配器
	 *
	 * 将不同返回类型统一映射为 bool：
	 *  - void   -> true
	 *  - 其它  -> static_cast<bool>
	 */
	template<typename F, typename... P>
	static bool InvokeWithBool(F&& f, P&&... p)
	{
		using Ret = std::invoke_result_t<F, P...>;
		if constexpr (std::is_void_v<Ret>) {
			std::invoke(std::forward<F>(f), std::forward<P>(p)...);
			return true;
		}
		else {
			return static_cast<bool>(
				std::invoke(std::forward<F>(f), std::forward<P>(p)...)
				);
		}
	}
};


/**
 * @brief DSL 规则构建器本体。
 *
 * RuleBuilder 负责：
 *  - 接收链式 DSL 调用
 *  - 填充 NotifyRule 各字段
 *  - 构建 delegateFactory（延迟构造）
 *  - 在 Commit 时将规则提交给 IRuleSink
 *
 * RuleBuilder 必须在栈上创建，
 * 以确保析构时 RAII 自动 Commit 生效。
 */
class RuleBuilder
{
public:
	/**
	 * @brief 构造函数
	 *
	 * @param sink 规则接收端（通常为 UIBindingRules）
	 */
	RuleBuilder(IRuleSink& sink);

	/**
	 * @brief 析构函数
	 *
	 * 若用户未显式调用 Commit()，
	 * 则在析构时自动提交规则。
	 */
	~RuleBuilder();

	/// 禁止在堆上创建，防止 RAII 失效
	void* operator new(size_t) = delete;

	// --------------------------------------------------------------------
	// DSL：控件匹配条件
	// --------------------------------------------------------------------

	/// 直接指定目标控件（跳过控件树匹配）
	RuleBuilder& Control(CControlUI* ctrl) noexcept
	{
		m_rule.control = ctrl;
		return *this;
	}

	/// 按控件名称匹配
	RuleBuilder& Name(LPCTSTR s) noexcept
	{
		m_rule.name = s;
		return *this;
	}

	/// 按控件类名匹配
	RuleBuilder& Class(LPCTSTR s) noexcept
	{
		m_rule.clazz = s;
		return *this;
	}

	/// 按分组匹配（Option / CheckBox）
	RuleBuilder& Group(LPCTSTR s) noexcept
	{
		m_rule.group = s;
		return *this;
	}

	/// 按 UserData Tag 匹配
	RuleBuilder& Tag(LPCTSTR s) noexcept
	{
		m_rule.tag = s;
		return *this;
	}

	/// 设置匹配标志位
	RuleBuilder& Flags(MatchFlag f) noexcept
	{
		m_rule.flags = f;
		return *this;
	}

	/// 设置匹配行为（All / Any）
	RuleBuilder& Behavior(MatchBehavior b) noexcept
	{
		m_rule.behavior = b;
		return *this;
	}

	/**
	 * @brief 设置前置过滤器
	 *
	 * 返回 false 时事件被拦截，
	 * 不会进入最终回调。
	 */
	RuleBuilder& Filter(UIEventFilter fn)
	{
		m_rule.userFilter = std::move(fn);
		return *this;
	}

	// --------------------------------------------------------------------
	// DSL：事件绑定入口
	// --------------------------------------------------------------------

	template <typename... T>
	RuleBuilder& OnClick(T&&... a)
	{
		return Bind(DUI_MSGTYPE_CLICK, std::forward<T>(a)...);
	}

	template <typename... T>
	RuleBuilder& OnSelectChanged(T&&... a)
	{
		return Bind(DUI_MSGTYPE_SELECTCHANGED, std::forward<T>(a)...);
	}

	template <typename... T>
	RuleBuilder& OnItemClick(T&&... a)
	{
		return Bind(DUI_MSGTYPE_ITEMCLICK, std::forward<T>(a)...);
	}

	template <typename... T>
	RuleBuilder& OnItemSelect(T&&... a)
	{
		return Bind(DUI_MSGTYPE_ITEMSELECT, std::forward<T>(a)...);
	}

	/**
	 * @brief 手动提交规则。
	 *
	 * 一般无需显式调用，
	 * 析构时会自动 Commit。
	 */
	void Commit();

private:
	/**
	 * @brief 核心绑定实现
	 *
	 * 将用户传入的回调及其参数
	 * 封装进 delegateFactory，
	 * 在 Commit 阶段再真正构造 delegate。
	 */
	template <typename... T>
	RuleBuilder& Bind(LPCTSTR type, T&&... args)
	{
		m_rule.eventType = type;

		m_rule.delegateFactory =
			[type, ...captured_args = std::forward<T>(args)]
		(UIEventFilter filter) mutable
		{
			auto wrapper = [=](TNotifyUI* msg) mutable -> bool {
				return DispatchCall(msg, captured_args...);
			};
			return std::make_shared<UniversalDelegate>(
				wrapper, type, filter
				);
		};
		return *this;
	}

	/**
	 * @brief 参数分发器
	 *
	 * 负责区分：
	 *  - (obj, &MemberFunc, args...)
	 *  - (func / lambda, args...)
	 */
	template <typename F, typename... Rest>
	static bool DispatchCall(TNotifyUI* msg, F&& f, Rest&&... rest)
	{
		if constexpr (sizeof...(rest) > 0) {
			using FirstRestType =
				std::tuple_element_t<0,
				std::tuple<std::decay_t<Rest>...>>;

			if constexpr (std::is_member_function_pointer_v<FirstRestType>) {
				// (obj, &member, ...)
				return EventInvoker::Call(
					std::forward<Rest>(rest)...,
					msg,
					std::forward<F>(f)
				);
			}
			else {
				// (callable, args...)
				return EventInvoker::Call(
					std::forward<F>(f),
					msg,
					std::forward<Rest>(rest)...
				);
			}
		}
		else {
			// (callable)
			return EventInvoker::Call(
				std::forward<F>(f),
				msg
			);
		}
	}

private:
	IRuleSink& m_sink;   ///< 规则接收端
	NotifyRule m_rule;   ///< 当前正在构建的规则
	bool       m_committed = false; ///< 是否已提交
};

} // namespace DolPP

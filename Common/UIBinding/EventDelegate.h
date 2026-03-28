// EventDelegate.h
#pragma once
#include <UIlib.h>
#include <functional>
#include <type_traits>

namespace DolPP
{

using namespace DuiLib;

/**
 * @brief UI 事件前置过滤器类型
 *
 * 返回 false 表示拦截事件，不进入最终回调；
 * 返回 true 表示继续处理。
 *
 * 该过滤器在 DuiLib 事件分发线程中同步执行，
 * 不应包含耗时或阻塞逻辑。
 */
using UIEventFilter = std::function<bool(TNotifyUI*)>;


/**
 * @brief UI 事件代理系统的基础抽象类（DuiLib 适配层）
 *
 * EventDelegateBase 是 DolPP 对 DuiLib::CDelegateBase 的封装，
 * 用于在 **不修改 DuiLib 源码** 的前提下，构建一套
 * 现代 C++（std::function / RAII / DSL）风格的事件绑定体系。
 *
 * ------------------------------------------------------------------
 * 【职责范围】
 * ------------------------------------------------------------------
 * 本类只负责“事件适配”，不涉及任何业务语义：
 *
 *  - 将 DuiLib 传入的 void* 转换为 TNotifyUI*
 *  - 校验事件有效性（空指针 / 无 sender）
 *  - 按事件类型进行过滤（click / selectchanged 等）
 *  - 执行用户提供的前置过滤器（可选）
 *  - 将事件转交给派生类执行最终逻辑
 *
 * ------------------------------------------------------------------
 * 【特别重要：关于 CDelegateBase 身份模型】
 * ------------------------------------------------------------------
 * DuiLib 使用 (m_pObject, m_pFn) 来判定 delegate 是否相等，
 * 且 Equals() **不可重写**。
 *
 * 因此：
 *  - EventDelegateBase 必须在构造时，向 CDelegateBase
 *    传入一个“稳定的身份标识”
 *  - 本系统约定：使用“原始 delegate 本体地址（this）”
 *    作为 m_pObject
 *
 * 这样可以确保：
 *  - DuiLib 在 Copy() 后生成的副本
 *  - 与原始 delegate 在 Equals() 意义上完全一致
 *  - -= 解绑操作可以精确命中并 delete 副本
 *
 * ------------------------------------------------------------------
 * 【生命周期说明】
 * ------------------------------------------------------------------
 *  - 原始 delegate（本体）由 DolPP 使用 shared_ptr 托管
 *  - Copy() 出来的 delegate 副本由 DuiLib 使用 new/delete 管理
 *  - 两者生命周期完全独立，不会发生双删或泄漏
 */
class EventDelegateBase : public CDelegateBase
{
public:
	/**
	 * @brief 构造基础事件委托
	 *
	 * @param eventType   需要匹配的 DuiLib 事件类型
	 *                    （如 DUI_MSGTYPE_CLICK）
	 * @param userFilter  用户自定义的前置过滤器（可为空）
	 * @param pObject     传递给 CDelegateBase 的身份指针，
	 *                    必须为“原始 delegate 本体的 this”
	 */
	EventDelegateBase(
		LPCTSTR eventType,
		UIEventFilter userFilter,
		void* pObject
	)
		// ⚠️ 关键点：
		// m_pObject 用于 DuiLib 的 Equals 判等，
		// 必须保持在 Copy 前后完全一致
		: CDelegateBase(pObject, nullptr)
		, m_eventType(eventType)
		, m_userFilter(std::move(userFilter))
	{
	}

		// ✅ 显式拷贝构造：确保 m_pObject 保持不变
	EventDelegateBase(const EventDelegateBase& other)
		: CDelegateBase(const_cast<EventDelegateBase&>(other).GetObject(), nullptr)
		, m_eventType(other.m_eventType)
		, m_userFilter(other.m_userFilter)
	{
	}

	/**
	 * @brief DuiLib 要求的虚拷贝接口
	 *
	 * += 绑定事件时，DuiLib 会调用 Copy()，
	 * 并接管返回对象的生命周期（new / delete）。
	 *
	 * 派生类必须返回一个“值语义等价”的副本，
	 * 且副本的 m_pObject 必须与原始对象一致。
	 */
	virtual CDelegateBase* Copy() const override = 0;

	/**
	 * @brief DuiLib 事件触发的统一入口
	 *
	 * 注意：
	 *  - 此函数在 UI 线程中被同步调用
	 *  - 不允许抛异常
	 *  - 返回值语义遵循 DuiLib 约定
	 */
	virtual bool Invoke(void* param) final
	{
		TNotifyUI* msg = reinterpret_cast<TNotifyUI*>(param);

		// 基础合法性检查
		if (!msg || !msg->pSender)
			return false;

		// 事件类型必须完全匹配
		if (m_eventType.IsEmpty() || msg->sType != m_eventType)
			return false;

		// 用户前置过滤器（可选）
		if (m_userFilter && !m_userFilter(msg))
			return false;

		// 进入最终业务逻辑
		return InvokeImpl(msg);
	}

protected:
	/**
	 * @brief 派生类实现的最终业务回调
	 *
	 * @param msg 已校验有效的事件参数
	 * @return 是否继续向下传播事件（DuiLib 语义）
	 */
	virtual bool InvokeImpl(TNotifyUI* msg) = 0;

protected:
	CDuiString   m_eventType;   ///< 事件类型过滤条件
	UIEventFilter m_userFilter; ///< 用户自定义前置过滤器
};


/**
 * @brief 通用事件代理实现
 *
 * UniversalDelegate 通过 std::function 封装任意可调用对象，
 * 是 RuleBuilder / DSL 系统最终生成的实际 delegate 类型。
 *
 * 支持：
 *  - Lambda
 *  - 成员函数
 *  - 全局函数
 *  - 任意可调用对象
 */
class UniversalDelegate : public EventDelegateBase
{
public:
	/**
	 * @brief 构造通用事件代理
	 *
	 * @param wrapper  已统一签名的最终回调函数
	 * @param type     事件类型（click / selectchanged 等）
	 * @param filter   前置过滤器
	 */
	UniversalDelegate(
		std::function<bool(TNotifyUI*)> wrapper,
		LPCTSTR type,
		UIEventFilter filter
	)
		// ⚠️ 核心设计点：
		// 将“原始 delegate 本体的 this”作为身份标识
		// 传递给 CDelegateBase
		: EventDelegateBase(type, std::move(filter), this)
		, m_wrapper(std::move(wrapper))
	{
	}

		// ✅ 拷贝构造：保持原始对象的 m_pObject
	UniversalDelegate(const UniversalDelegate& other)
		: EventDelegateBase(other)
		, m_wrapper(other.m_wrapper)
	{
	}

	/**
	 * @brief DuiLib 要求的拷贝实现
	 *
	 * Copy 出来的副本：
	 *  - 地址不同
	 *  - 但 m_pObject 与原始对象一致
	 *
	 * 从而保证：
	 *  - += / -= 可以正确匹配
	 *  - 副本由 DuiLib delete
	 *  - 原始对象由 DolPP 托管
	 */
	virtual CDelegateBase* Copy() const override
	{
		return new UniversalDelegate(*this);
	}

protected:
	virtual bool InvokeImpl(TNotifyUI* msg) override
	{
		return m_wrapper(msg);
	}

private:
	std::function<bool(TNotifyUI*)> m_wrapper; ///< 实际执行业务逻辑的回调
};

} // namespace DolPP

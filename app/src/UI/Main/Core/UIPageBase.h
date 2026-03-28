// UIPageBase.h
#pragma once
#include "Common/UIBinding/UIBindingRules.h"
#include <atomic>
#include <UIlib.h>

namespace DolPP
{

using namespace DuiLib;

/**
 * @brief UIPageBase —— 页面级 UI 管理基类。
 *
 * UIPageBase 是一个“页面级生命周期容器”，
 * 用于管理：
 *  - 页面根控件（Root Control）
 *  - 页面级事件绑定（UIBindingRules）
 *  - 页面初始化 / 反初始化流程
 *
 * ------------------------------------------------------------------
 * 【核心设计目标】
 * ------------------------------------------------------------------
 * 1. 为“一个页面”提供清晰、统一的生命周期模型：
 *
 *      构造 → Initialize → 使用中 → Uninitialize → 析构
 *
 * 2. 将事件绑定逻辑集中托管，避免：
 *    - 继承 CNotifyPump
 *    - 在窗口类中写大量 if / switch
 *
 * 3. 让派生类只关注“页面本身”：
 *    - OnBind()     ：声明事件绑定规则
 *    - OnInit()     ：初始化 UI 状态
 *
 * ------------------------------------------------------------------
 * 【重要约束】
 * ------------------------------------------------------------------
 * - UIPageBase 不允许拷贝或移动
 * - Initialize / Uninitialize 为幂等操作
 * - 所有 UI 操作必须发生在 UI 线程
 */
class UIPageBase
{
public:
	/**
	 * @brief 工厂函数：创建并初始化页面对象。
	 *
	 * 该函数用于强制规范页面的创建流程：
	 *
	 *   1. 使用 new 创建派生类对象
	 *   2. 在派生类构造完成后，立即调用 Initialize()
	 *
	 * 设计意图：
	 *  - 避免在构造函数中调用虚函数
	 *  - 确保 OnBind / OnInit 的调用时机正确
	 *
	 * @tparam T    UIPageBase 的派生类型
	 * @tparam Args 构造函数参数
	 */
	template<typename T, typename... Args>
	static T* CreatePage(Args&&... args)
	{
		static_assert(std::is_base_of_v<UIPageBase, T>,
			"T must inherit from UIPageBase");

		T* page = new T(std::forward<Args>(args)...);
		page->Initialize();
		return page;
	}

	/// 禁止拷贝和赋值，页面对象必须具有唯一生命周期
	UIPageBase(const UIPageBase&) = delete;
	UIPageBase& operator=(const UIPageBase&) = delete;

protected:
	/**
	 * @brief 使用已知 root 控件指针构造页面。
	 *
	 * @param mgr  DuiLib 的 PaintManager
	 * @param root 页面根控件
	 *
	 * 注意：
	 *  - 构造阶段不执行任何绑定或初始化
	 *  - 不调用虚函数
	 */
	UIPageBase(CPaintManagerUI* mgr, CControlUI* root);

	/**
	 * @brief 通过 root 控件名称查找并构造页面。
	 *
	 * @param mgr      PaintManager
	 * @param rootName 根控件名称
	 *
	 * 若未找到对应控件，root 将为空，
	 * 页面后续初始化将安全地退化为空操作。
	 */
	UIPageBase(CPaintManagerUI* mgr, LPCTSTR rootName);

	/**
	 * @brief 虚析构函数。
	 *
	 * 析构时会自动执行 Uninitialize()，
	 * 确保页面销毁前已解除所有事件绑定。
	 */
	virtual ~UIPageBase();

	/**
	 * @brief 页面事件绑定声明点（DSL 构建阶段）。
	 *
	 * 派生类应在此函数中：
	 *  - 使用 BindingRules() 构建事件绑定规则
	 *  - 不进行任何 UI 状态修改
	 *
	 * 该函数只会在 Initialize() 中被调用一次。
	 */
	virtual void OnBind() {}

	/**
	 * @brief 页面初始化逻辑。
	 *
	 * 用于：
	 *  - 查找控件
	 *  - 设置初始状态
	 *  - 初始化成员变量
	 *
	 * 调用时机：
	 *  - 所有事件绑定已完成之后
	 */
	virtual void OnInit() {}

protected:
	// ============================================================
	// 工具函数 —— 控件查找 / 遍历
	// ============================================================

	/**
	 * @brief 根据控件名称查找子控件（类型安全版本）。
	 *
	 * @tparam T   期望的控件类型
	 * @param name 控件名称
	 * @return     找到的控件指针，或 nullptr
	 */
	template<typename T>
	T* Find(LPCTSTR name)
	{
		if (!m_pManager || !m_pRoot)
			return nullptr;

		return static_cast<T*>(
			m_pManager->FindSubControlByName(m_pRoot, name)
			);
	}

	/**
	 * @brief 便捷宏：在页面根控件下查找子控件。
	 *
	 * 仅作为语法糖使用，不参与核心逻辑。
	 */
#define FIND_SUBCTRL(clazz,name) \
		static_cast<clazz*>(FindSubControl(GetRoot(), _T(name)))

	/**
	 * @brief 从指定父控件开始，递归查找名称匹配的子控件。
	 *
	 * 使用 DFS（深度优先）遍历控件树。
	 *
	 * @param parent 起始控件
	 * @param name   目标控件名称
	 */
	CControlUI* FindSubControl(CControlUI* parent, const LPCTSTR name)
	{
		if (!parent)
			return nullptr;

		// 1. 判断是否为容器控件
		auto container =
			static_cast<CContainerUI*>(parent->GetInterface(_T("Container")));
		if (!container)
			return nullptr;

		// 2. 遍历当前容器的直接子控件
		for (int i = 0; i < container->GetCount(); ++i) {
			CControlUI* ctrl = container->GetItemAt(i);

			// 2a. 当前控件名称匹配
			if (ctrl->GetName() == name)
				return ctrl;

			// 2b. 递归搜索子树
			CControlUI* found = FindSubControl(ctrl, name);
			if (found)
				return found;
		}

		// 3. 整棵子树均未找到
		return nullptr;
	}

	/**
	 * @brief 对指定控件及其所有子控件执行回调函数。
	 *
	 * @tparam Func 可调用对象，签名为 void(CControlUI*)
	 */
	template<typename Func>
	void ForEachControl(CControlUI* ctrl, Func fn)
	{
		if (!ctrl)
			return;

		fn(ctrl);

		auto container =
			static_cast<CContainerUI*>(ctrl->GetInterface(_T("Container")));
		if (!container)
			return;

		for (int i = 0; i < container->GetCount(); ++i)
			ForEachControl(container->GetItemAt(i), fn);
	}

	/**
	 * @brief 对整个页面的控件树执行遍历操作。
	 */
	template<typename Func>
	void ForEach(Func fn)
	{
		ForEachControl(m_pRoot, fn);
	}

	/// 获取 PaintManager
	CPaintManagerUI* GetManager() const { return m_pManager; }

	/// 获取页面根控件
	CControlUI* GetRoot() const { return m_pRoot; }

	/// 获取页面根控件名称
	LPCTSTR GetRootName() const { return m_szRootName; }

	/**
	 * @brief 获取事件绑定系统（DSL 入口）。
	 *
	 * 派生类应仅在 OnBind() 中使用该接口。
	 */
	UIBindingRules& BindingRules() { return m_bindingRules; }

private:
	/**
	 * @brief 页面初始化入口（幂等）。
	 *
	 * 调用顺序：
	 *  1. OnBind()
	 *  2. ApplyRules()
	 *  3. OnInit()
	 */
	void Initialize();

	/**
	 * @brief 页面反初始化（幂等）。
	 *
	 * 解除所有事件绑定，防止悬空回调。
	 */
	void Uninitialize();

private:
	CPaintManagerUI* m_pManager = nullptr;		///< DuiLib 管理器
	CControlUI* m_pRoot = nullptr;				///< 页面根控件，界定了当前 Page 的控制边界
	LPCTSTR          m_szRootName = nullptr;	///< 根控件名称

	UIBindingRules   m_bindingRules;			///< 页面级事件绑定系统
	std::atomic_bool m_initialized{ false };	///< 初始化幂等标记
};

} // namespace DolPP

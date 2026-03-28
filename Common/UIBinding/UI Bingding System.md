# DolPP UI Binding System  
（DuiLib 现代化事件绑定系统）

DolPP UI Binding System 为 DuiLib 提供 **现代化、类型安全、支持 Lambda 的 UI 事件绑定能力**，完全替代传统 `Notify()` 分发，使 UI 代码更简洁、更安全、更容易维护。

---

# ✨ 设计目标

## ✔ 1. 完全替代 Notify() 消息分发

传统方式：

```cpp
void Notify(TNotifyUI& msg) {
    if (msg.sType == DUI_MSGTYPE_CLICK &&
        msg.pSender->GetName() == _T("btn_ok"))
    {
        ...
    }
}
```

现代方式：

```cpp
BindingRules().AddRule()
    .Name(_T("btn_ok"))
    .Flags(MatchFlag::Name)
    .OnClick(this, &MyPage::OnOkClicked);
```

---

## ✔ 2. 支持所有常用的回调格式

- 成员函数（成员方法指针）
- lambda
- functor（重载 operator() 的对象）
- `std::function<bool(TNotifyUI*)>`

示例：

```cpp
.OnClick([](TNotifyUI* msg){ ... })
.OnClick(this, &MyClass::Fn)
.OnClick(std::bind(&MyClass::Fn, this, std::placeholders::_1))
```

---

## ✔ 3. 强大的控件匹配 DSL

可按以下特征自动匹配控件：

- Name  
- Class  
- Group  
- Tag  
- UserData 不为空  
- Visible / Enabled  
- Container  
- 或直接绑定控件指针

例如：

```cpp
BindingRules().AddRule()
    .Class(_T("ComboUI"))
    .Flags(MatchFlag::Class)
    .OnSelectChanged(this, &MyPage::OnComboChanged);
```

---

## ✔ 4. 自动生命周期管理

- 自动解绑（UnapplyRules）
- 自动托管委托（unique_ptr）
- 避免悬空回调
- 零手动 delete

---

## ✔ 5. 不需要 AddNotifier / NotifyPump

不会造成全局消息污染  
每个控件只接收自己绑定的消息。

---

# 🧩 核心模块介绍

---

## 1. `NotifyRule`

NotifyRule 是一个纯数据结构（无逻辑），描述一条绑定规则，包括：

- 匹配字段（name/class/group/tag/control）
- 匹配行为（Any / All）
- 事件类型（Click / SelectChanged / ItemSelect...）
- 回调指针
- 用户自定义 Filter

---

## 2. `EventDelegateBase`

统一封装 DuiLib Delegate 机制：

- 自动把 void* 转换为 `TNotifyUI*`
- 自动检查事件类型（msg.sType）
- 调用用户 Filter
- 再调用用户事件回调（lambda 或成员函数）

派生类型：

- `MemberEventDelegate<T>`  
- `LambdaEventDelegate<Func>`

---

## 3. `RuleBuilder`（DSL）

链式 API：

```cpp
BindingRules().AddRule()
    .Name(_T("btn_close"))
    .Flags(MatchFlag::Name)
    .Filter([](TNotifyUI* msg){ return true; })
    .OnClick(this, &MyPage::OnClose);
```

### ⚠ 必须遵守 DSL 顺序规则：

- Filter 必须写在事件绑定 **之前**
- OnClick / OnSelectChanged 必须写在 DSL **最后一步**

正确：

```cpp
.Filter(fn)
.OnClick(fn);
```

错误（Filter 不会生效）：

```cpp
.OnClick(fn)
.Filter(fn);   // ❌ 无效
```

原因：  
OnClick() 会立即创建 Delegate，之后添加 Filter 已经太迟。

---

## 4. `UIBindingRules`

负责：

- 保存所有规则
- 遍历 UI 树并匹配控件
- 执行绑定 / 解绑
- 托管委托生命周期

支持两类绑定：

### 控件匹配绑定

```cpp
.Name().Class().Flags().OnClick()
```

### 直接控件绑定

```cpp
.Control(btn)
.OnClick(...)
```

---

## 5. `UIPageBase`

统一管理 UI 页面生命周期：

### Initialize():

1. BuildNotifyRules()
2. ApplyRules()
3. Init()
4. OnInitialized()

### Uninitialize():

- 自动解绑
- 可选移除 delegates

典型用法：

```cpp
class NavPage : public UIPageBase {

    NavPage(...) : UIPageBase(mgr, root) {
        Initialize();
    }

    void BuildNotifyRules() override {
        BindingRules().AddRule()
            .Name(_T("nav_btn"))
            .Flags(MatchFlag::Name)
            .OnClick(this, &NavPage::OnNavClick);
    }
};
```

---

# 🛠️ 使用示例

---

## ✔ 示例 1：按 Name 绑定 Click

```cpp
BindingRules().AddRule()
    .Name(_T("btn_ok"))
    .Flags(MatchFlag::Name)
    .OnClick(this, &MyPage::OnOk);
```

---

## ✔ 示例 2：绑定 SelectChanged（只在选中时触发）

```cpp
BindingRules().AddRule()
    .Class(_T("OptionUI"))
    .Flags(MatchFlag::Class)
    .Filter([](TNotifyUI* msg) {
        return static_cast<COptionUI*>(msg->pSender)->IsSelected();
    })
    .OnSelectChanged(this, &MyPage::OnOptionChanged);
```

---

## ✔ 示例 3：Lambda 回调

```cpp
BindingRules().AddRule()
    .Name(_T("btn_close"))
    .Flags(MatchFlag::Name)
    .OnClick([](TNotifyUI* msg){
        ::PostQuitMessage(0);
        return true;
    });
```

---

## ✔ 示例 4：直接绑定控件指针

```cpp
BindingRules().AddRule()
    .Control(m_btnRefresh)
    .OnClick(this, &MyPage::OnRefresh);
```

---

# 🔥 优点总结

- 无 NotifyPump  
- 无 AddNotifier  
- 无 msg.sType 手工判断  
- DSL 高度可读和可维护  
- 完整生命周期托管  
- 现代 C++（lambda / functor / std::function 全支持）  
- UI 逻辑集中、清晰、稳定  

---

# 📌 注意事项

### 1. Initialize() 必须在派生类构造结束后调用
否则无法正确构建规则。

### 2. Filter 必须写在事件绑定前  
否则不会生效。

### 3. OnClick/OnSelectChanged 必须是 DSL 最后一步

### 4. Uninitialize() 建议在页面卸载时调用

---

# 📚 总结

DolPP UI Binding System 为 DuiLib 提供：

- 现代事件绑定模型  
- 强类型 DSL  
- 自动过滤  
- 自动生命周期管理  
- 零 NotifyPump  
- 零全局消息污染  

这使得 DuiLib 升级为一个 **现代、可维护、可扩展** 的 UI 框架。

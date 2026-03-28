/*
 * AIR.h  —  Abstract Intermediate Representation
 *
 * 游戏宏跨平台转换工具 —— 中间表示层定义
 *
 * ============================================================================
 * 版本历史
 * ============================================================================
 *
 * v0.1  初始方案讨论稿
 *
 * v0.2  根据评审意见修订（继承体系、unique_ptr、新增节点等）
 *
 * v0.3  修复全局宏命名冲突：
 *
 * 问题根源：
 * C 预处理器的宏替换无视 namespace 和 enum class 的作用域保护。
 * 只要宏存在，enum class 成员的名称文本就会被替换，导致编译错误。
 *
 * 受影响的标识符及来源：
 *
 * 标识符             冲突来源
 * ─────────────  ──────────────────────────────────────────────────────
 * RELATIVE        wingdi.h / mmsystem.h：#define RELATIVE 0
 * ABSOLUTE        wingdi.h / mmsystem.h：#define ABSOLUTE 1
 * MOD             <cmath> 某些平台实现定义了 MOD 宏
 * AND             <iso646.h> 在部分编译器中将 AND 定义为 &&
 * OR              <iso646.h> 在部分编译器中将 OR  定义为 ||
 * NOT             <iso646.h> 在部分编译器中将 NOT 定义为 !
 * MOD_SHIFT       SDL2 SDL_Keymod、部分 X11 封装
 * MOD_ALT         SDL2 SDL_Keymod、GTK GdkModifierType
 * MOD_CTRL        部分 UI 框架
 * MOD_NONE        部分 UI 框架
 * MOD_WIN         部分框架
 *
 * 修复方案（统一原则，不打零散补丁）：
 * 1. CoordType   枚举值加 Coord 前缀：CoordRelative / CoordAbsolute
 * 2. ModifierMask 改为 enum class ModMask，枚举值去掉 MOD_ 前缀，
 * 补充 operator| / operator& / hasModifier 支持位运算
 * 3. BinOp 中 MOD/AND/OR 改为 OpMod/OpAnd/OpOr
 * 4. UnOp  中 NOT        改为 OpNot
 *
 * ============================================================================
 * 插件编译约定（重要！所有插件开发者必读）
 * ============================================================================
 *
 * AIRNode 对象的内存在主程序堆和插件 DLL 之间传递。
 * 为避免跨 CRT 的堆损坏（Heap Corruption），所有插件必须：
 *
 * 1. 使用与主程序相同版本的 Visual Studio 编译
 * 2. 运行时库选择"多线程 DLL"（/MD 或 /MDd），
 * 禁止使用静态链接 CRT（/MT 或 /MTd）
 * 3. 字符集统一使用 Unicode（/D UNICODE）
 *
 * 违反以上约定将导致运行时堆损坏崩溃，且错误极难定位。
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace AIR {

    // ============================================================================
    //  前向声明 (Forward Declarations)
    // ============================================================================
    // 解决 C2027: 必须先声明所有节点类，IAIRVisitor 才能定义其 visit 虚函数。

    class IAIRVisitor;

    struct AIRNode;
    struct ProgramNode;
    struct SequenceNode;
    struct FunctionDefNode;
    struct FunctionCallNode;
    struct TriggerNode;
    struct HotkeyNode;
    struct IfNode;
    struct LoopCountNode;
    struct LoopWhileNode;
    struct LoopDoWhileNode;
    struct LoopInfiniteNode;
    struct BreakNode;
    struct ContinueNode;
    struct ReturnNode;
    struct LabelNode;
    struct GotoNode;
    struct VarAssignNode;
    struct VarDeclareNode;
    struct KeyDownNode;
    struct KeyUpNode;
    struct KeyTapNode;
    struct KeySendStringNode;
    struct MouseMoveNode;
    struct MouseClickNode;
    struct MouseDownNode;
    struct MouseUpNode;
    struct MouseScrollNode;
    struct SleepNode;
    struct QueryKeyStateNode;
    struct ExprNumberNode;
    struct ExprStringNode;
    struct ExprBoolNode;
    struct ExprVarNode;
    struct ExprCallNode;
    struct ExprBinopNode;
    struct ExprUnopNode;
    struct ExprTernaryNode;
    struct ExprRandomRangeNode;
    struct CommentNode;
    struct RawNode;

    // AIRNodePtr：unique_ptr 管理节点生命周期，遍历时使用裸指针 AIRNode*
    using AIRNodePtr = std::unique_ptr<AIRNode>;


    // ============================================================================
    //  §1  统一键名枚举  AIRKey
    // ============================================================================
    //
    // AIR 内部统一使用此枚举，彻底消除平台键名差异。
    // Parse  插件：将平台键名 → AIRKey
    // Generate 插件：将 AIRKey → 平台键名
    // 无法识别的键使用 KEY_UNKNOWN，rawKey 字段保留原始字符串。
    //

    enum class AIRKey : uint16_t {

        // ── 鼠标按键 ──────────────────────────────────────────────────────────
        KEY_MOUSE_LEFT,
        KEY_MOUSE_RIGHT,
        KEY_MOUSE_MIDDLE,
        KEY_MOUSE_X1,
        KEY_MOUSE_X2,

        // ── 字母键 ────────────────────────────────────────────────────────────
        KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
        KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
        KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
        KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

        // ── 数字键（主键盘行）─────────────────────────────────────────────────
        KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
        KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,

        // ── 数字小键盘 ────────────────────────────────────────────────────────
        KEY_NUMPAD0, KEY_NUMPAD1, KEY_NUMPAD2, KEY_NUMPAD3, KEY_NUMPAD4,
        KEY_NUMPAD5, KEY_NUMPAD6, KEY_NUMPAD7, KEY_NUMPAD8, KEY_NUMPAD9,
        KEY_NUMPAD_DOT,
        KEY_NUMPAD_ADD,
        KEY_NUMPAD_SUB,
        KEY_NUMPAD_MUL,
        KEY_NUMPAD_DIV,
        KEY_NUMPAD_ENTER,
        KEY_NUMLOCK,

        // ── 功能键 ────────────────────────────────────────────────────────────
        KEY_F1, KEY_F2, KEY_F3, KEY_F4,
        KEY_F5, KEY_F6, KEY_F7, KEY_F8,
        KEY_F9, KEY_F10, KEY_F11, KEY_F12,
        KEY_F13, KEY_F14, KEY_F15, KEY_F16,
        KEY_F17, KEY_F18, KEY_F19, KEY_F20,
        KEY_F21, KEY_F22, KEY_F23, KEY_F24,

        // ── 修饰键 ────────────────────────────────────────────────────────────
        KEY_CTRL,
        KEY_LCTRL,
        KEY_RCTRL,
        KEY_SHIFT,
        KEY_LSHIFT,
        KEY_RSHIFT,
        KEY_ALT,
        KEY_LALT,
        KEY_RALT,
        KEY_WIN,
        KEY_LWIN,
        KEY_RWIN,

        // ── 导航键 ────────────────────────────────────────────────────────────
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_HOME,
        KEY_END,
        KEY_PGUP,
        KEY_PGDN,
        KEY_INSERT,
        KEY_DELETE,

        // ── 编辑键 ────────────────────────────────────────────────────────────
        KEY_ENTER,
        KEY_BACKSPACE,
        KEY_TAB,
        KEY_SPACE,
        KEY_ESCAPE,
        KEY_CAPSLOCK,
        KEY_SCROLLLOCK,
        KEY_PAUSE,
        KEY_PRINTSCREEN,

        // ── 符号键（美式键盘布局）────────────────────────────────────────────
        KEY_BACKTICK,    // `  ~
        KEY_MINUS,       // -  _
        KEY_EQUALS,      // =  +
        KEY_LBRACKET,    // [  {
        KEY_RBRACKET,    // ]  }
        KEY_BACKSLASH,   // \  |
        KEY_SEMICOLON,   // ;  :
        KEY_QUOTE,       // '  "
        KEY_COMMA,       // ,  <
        KEY_DOT,         // .  >
        KEY_SLASH,       // /  ?

        // ── 媒体键 ────────────────────────────────────────────────────────────
        KEY_MEDIA_PLAY_PAUSE,
        KEY_MEDIA_STOP,
        KEY_MEDIA_NEXT,
        KEY_MEDIA_PREV,
        KEY_VOLUME_UP,
        KEY_VOLUME_DOWN,
        KEY_VOLUME_MUTE,

        // ── 特殊值 ────────────────────────────────────────────────────────────
        KEY_UNKNOWN,
    };


    // ============================================================================
    //  §2  辅助枚举
    // ============================================================================

    // ── 鼠标滚轮方向 ─────────────────────────────────────────────────────────────
    enum class ScrollDir {
        Up,     // 向上滚
        Down,   // 向下滚
        Left,   // 向左滚（横向滚轮）
        Right,  // 向右滚
    };

    // ── 坐标系类型 ───────────────────────────────────────────────────────────────
    //
    // v0.3 修复：
    //    原 RELATIVE / ABSOLUTE 与 wingdi.h / mmsystem.h 的同名宏冲突。
    //    宏的文本替换无视 enum class 作用域，会导致以下编译错误：
    //      enum class CoordType { RELATIVE, ABSOLUTE }
    //      → 被展开为 enum class CoordType { 0, 1 }  （编译器报 expected identifier）
    //
    //    修复方案：枚举值改用 Coord 前缀，彻底回避宏名称。
    //
    enum class CoordType {
        CoordRelative,   // 相对当前位置偏移（压枪宏最常用）
        CoordAbsolute,   // 移动到屏幕绝对坐标
    };

    // ── 修饰键掩码 ───────────────────────────────────────────────────────────────
    //
    // v0.3 修复：
    //    原 enum ModifierMask（普通 enum）中的 MOD_SHIFT / MOD_ALT / MOD_CTRL
    //    与 SDL2（SDL_Keymod）、GTK（GdkModifierType）等常见库冲突。
    //    普通 enum 成员泄漏到外层作用域，即使在 namespace AIR 内，
    //    一旦使用者 "using namespace AIR" 即产生冲突。
    //
    //    修复方案：
    //      1. 改为 enum class ModMask（成员限定在 ModMask:: 作用域内）
    //      2. 枚举值去掉 MOD_ 前缀，改用首字母大写风格
    //      3. 补充 operator| / operator& / hasModifier，保持位运算可用性
    //
    enum class ModMask : uint8_t {
        None = 0x00,
        Ctrl = 0x01,
        Shift = 0x02,
        Alt = 0x04,
        Win = 0x08,
    };

    // enum class 不支持原生 | & 运算，需要显式重载
    inline ModMask operator|(ModMask a, ModMask b) noexcept {
        return static_cast<ModMask>(
            static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }
    inline ModMask operator&(ModMask a, ModMask b) noexcept {
        return static_cast<ModMask>(
            static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }
    inline ModMask& operator|=(ModMask& a, ModMask b) noexcept {
        a = a | b; return a;
    }
    // 检查某个修饰键是否被设置（替代原来的 mods & MOD_CTRL 写法）
    // 用法：if (hasModifier(mods, ModMask::Ctrl)) { ... }
    inline bool hasModifier(ModMask mask, ModMask flag) noexcept {
        return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(flag)) != 0;
    }

    // ── 触发事件类型 ─────────────────────────────────────────────────────────────
    enum class TriggerEvent {
        KeyDown,          // 键盘按下
        KeyUp,            // 键盘松开
        MouseButtonDown,  // 鼠标按下
        MouseButtonUp,    // 鼠标松开
        MacroStart,       // 宏开始执行（罗技 MACRO_START_EVENT）
        MacroStop,        // 宏停止执行（罗技 MACRO_STOP_EVENT）
    };

    // ── 二元运算符 ───────────────────────────────────────────────────────────────
    //
    // v0.3 修复：
    //    MOD → OpMod：<cmath> 在部分平台定义了 MOD 宏
    //    AND → OpAnd：<iso646.h> 在部分编译器中 AND 被定义为 &&
    //    OR  → OpOr ：<iso646.h> 在部分编译器中 OR  被定义为 ||
    //
    //    注：<iso646.h> 影响范围：
    //      MSVC 在 /Za（严格标准）模式下会将 and/or/not 等作为关键字，
    //      而大写的 AND/OR/NOT 在某些第三方头文件或宏定义中也有使用。
    //      使用 Op 前缀是最保险的做法。
    //
    enum class BinOp {
        Add,    // +
        Sub,    // -
        Mul,    // *
        Div,    // /
        OpMod,  // %（取余）   ← 原 MOD，回避 <cmath> 宏
        Pow,    // **（幂次）

        Eq,     // ==
        Neq,    // !=
        Lt,     // <
        Gt,     // >
        Lte,    // <=
        Gte,    // >=

        OpAnd,  // &&（逻辑与）← 原 AND，回避 <iso646.h> 宏
        OpOr,   // ||（逻辑或）← 原 OR，回避 <iso646.h> 宏

        Concat, // 字符串拼接（Lua 的 .. / AHK 的 .）
    };

    // ── 一元运算符 ───────────────────────────────────────────────────────────────
    //
    // v0.3 修复：
    //    NOT → OpNot：<iso646.h> 在部分编译器中 NOT 被定义为 !
    //
    enum class UnOp {
        OpNot,  // !（逻辑非）← 原 NOT，回避 <iso646.h> 宏
        Neg,    // -（数值取反）
    };

    // ── 变量作用域 ───────────────────────────────────────────────────────────────
    enum class VarScope {
        Local,   // 局部变量（Lua local / AHK v2 默认）
        Global,  // 全局变量（AHK global / Lua 默认）
        Static,  // 静态变量（AHK static，多次调用间保持值）
    };


    // ============================================================================
    //  §3  节点类型枚举  AIRNodeType
    // ============================================================================

    enum class AIRNodeType {

        // ── 程序结构 ──────────────────────────────────────────────────────────
        Program,
        Sequence,
        FunctionDef,
        FunctionCall,

        // ── 触发器 / 热键 ─────────────────────────────────────────────────────
        Trigger,
        Hotkey,

        // ── 控制流 ────────────────────────────────────────────────────────────
        If,
        LoopCount,
        LoopWhile,
        LoopDoWhile,
        LoopInfinite,
        Break,
        Continue,
        Return,
        Label,
        Goto,

        // ── 变量 ──────────────────────────────────────────────────────────────
        VarAssign,
        VarDeclare,

        // ── 键盘动作 ──────────────────────────────────────────────────────────
        KeyDown,
        KeyUp,
        KeyTap,
        KeySendString,

        // ── 鼠标动作 ──────────────────────────────────────────────────────────
        MouseMove,
        MouseClick,
        MouseDown,
        MouseUp,
        MouseScroll,

        // ── 时间 ──────────────────────────────────────────────────────────────
        Sleep,

        // ── 查询 ──────────────────────────────────────────────────────────────
        QueryKeyState,

        // ── 表达式 ────────────────────────────────────────────────────────────
        ExprNumber,
        ExprString,
        ExprBool,
        ExprVar,
        ExprCall,
        ExprBinop,
        ExprUnop,
        ExprTernary,
        ExprRandomRange,

        // ── 元信息 ────────────────────────────────────────────────────────────
        Comment,
        Raw,
    };


    // ============================================================================
    //  §4  节点基类  AIRNode
    // ============================================================================

    struct AIRNode {
        AIRNodeType                type;
        std::vector<AIRNodePtr>     children;
        int                         srcLine = 0;
        std::string                 srcFile;

        explicit AIRNode(AIRNodeType t) : type(t) {}
        virtual ~AIRNode() = default;

        AIRNode(const AIRNode&) = delete;
        AIRNode& operator=(const AIRNode&) = delete;
        AIRNode(AIRNode&&) = default;
        AIRNode& operator=(AIRNode&&) = default;

        AIRNode& addChild(AIRNodePtr child) {
            children.push_back(std::move(child));
            return *this;
        }

        virtual void accept(IAIRVisitor& visitor) = 0;
    };


    // ============================================================================
    //  §5  派生节点类
    // ============================================================================

    // 修复 C2027: 宏展开改为仅声明，函数体移至文件末尾 §10
#define AIR_ACCEPT_IMPL \
    void accept(IAIRVisitor& v) override;


// ────────────────────────────────────────────────────────────────────────────
//  程序结构
// ────────────────────────────────────────────────────────────────────────────

    struct ProgramNode : AIRNode {
        ProgramNode() : AIRNode(AIRNodeType::Program) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make() { return std::make_unique<ProgramNode>(); }
    };

    struct SequenceNode : AIRNode {
        SequenceNode() : AIRNode(AIRNodeType::Sequence) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make() { return std::make_unique<SequenceNode>(); }
    };

    struct FunctionDefNode : AIRNode {
        std::string                name;
        std::vector<std::string> params;
        FunctionDefNode() : AIRNode(AIRNodeType::FunctionDef) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string name,
                std::vector<std::string> params = {}) {
            auto n = std::make_unique<FunctionDefNode>();
            n->name = std::move(name);
            n->params = std::move(params);
            return n;
        }
    };

    struct FunctionCallNode : AIRNode {
        std::string name;
        FunctionCallNode() : AIRNode(AIRNodeType::FunctionCall) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string name) {
            auto n = std::make_unique<FunctionCallNode>();
            n->name = std::move(name);
            return n;
        }
    };


    // ────────────────────────────────────────────────────────────────────────────
    //  触发器 / 热键
    // ────────────────────────────────────────────────────────────────────────────

    // children[0]: SequenceNode（触发时执行的动作）
    struct TriggerNode : AIRNode {
        TriggerEvent triggerEvent = TriggerEvent::MouseButtonDown;
        AIRKey       key = AIRKey::KEY_MOUSE_LEFT;
        ModMask      modifiers = ModMask::None;
        TriggerNode() : AIRNode(AIRNodeType::Trigger) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(TriggerEvent evt, AIRKey key,
                ModMask mods = ModMask::None) {
            auto n = std::make_unique<TriggerNode>();
            n->triggerEvent = evt;
            n->key = key;
            n->modifiers = mods;
            return n;
        }
    };

    // children[0]: SequenceNode
    struct HotkeyNode : AIRNode {
        AIRKey   key = AIRKey::KEY_UNKNOWN;
        ModMask  modifiers = ModMask::None;
        bool     passthrough = false;
        HotkeyNode() : AIRNode(AIRNodeType::Hotkey) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(AIRKey key,
                ModMask mods = ModMask::None,
                bool passthrough = false) {
            auto n = std::make_unique<HotkeyNode>();
            n->key = key;
            n->modifiers = mods;
            n->passthrough = passthrough;
            return n;
        }
    };


    // ────────────────────────────────────────────────────────────────────────────
    //  控制流
    // ────────────────────────────────────────────────────────────────────────────

    // children[0]: 条件表达式
    // children[1]: then 分支 SequenceNode
    // children[2]: else 分支 SequenceNode（可选）
    struct IfNode : AIRNode {
        IfNode() : AIRNode(AIRNodeType::If) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make() { return std::make_unique<IfNode>(); }
        bool hasElse() const { return children.size() >= 3; }
    };

    // children[0]: 次数表达式
    // children[1]: 循环体 SequenceNode
    struct LoopCountNode : AIRNode {
        LoopCountNode() : AIRNode(AIRNodeType::LoopCount) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make() { return std::make_unique<LoopCountNode>(); }
    };

    // children[0]: 条件表达式（为真则继续）
    // children[1]: 循环体 SequenceNode
    struct LoopWhileNode : AIRNode {
        LoopWhileNode() : AIRNode(AIRNodeType::LoopWhile) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make() { return std::make_unique<LoopWhileNode>(); }
    };

    // children[0]: 循环体 SequenceNode（至少执行一次）
    // children[1]: 退出条件表达式（为真时退出，即 until 语义）
    struct LoopDoWhileNode : AIRNode {
        LoopDoWhileNode() : AIRNode(AIRNodeType::LoopDoWhile) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make() { return std::make_unique<LoopDoWhileNode>(); }
    };

    // children[0]: 循环体 SequenceNode
    struct LoopInfiniteNode : AIRNode {
        LoopInfiniteNode() : AIRNode(AIRNodeType::LoopInfinite) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make() { return std::make_unique<LoopInfiniteNode>(); }
    };

    struct BreakNode : AIRNode {
        BreakNode() : AIRNode(AIRNodeType::Break) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make() { return std::make_unique<BreakNode>(); }
    };

    struct ContinueNode : AIRNode {
        ContinueNode() : AIRNode(AIRNodeType::Continue) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make() { return std::make_unique<ContinueNode>(); }
    };

    // children[0]: 返回值表达式（可选）
    struct ReturnNode : AIRNode {
        ReturnNode() : AIRNode(AIRNodeType::Return) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make() { return std::make_unique<ReturnNode>(); }
        bool hasValue() const { return !children.empty(); }
    };

    struct LabelNode : AIRNode {
        std::string name;
        LabelNode() : AIRNode(AIRNodeType::Label) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string name) {
            auto n = std::make_unique<LabelNode>();
            n->name = std::move(name);
            return n;
        }
    };

    struct GotoNode : AIRNode {
        std::string target;
        GotoNode() : AIRNode(AIRNodeType::Goto) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string target) {
            auto n = std::make_unique<GotoNode>();
            n->target = std::move(target);
            return n;
        }
    };


    // ────────────────────────────────────────────────────────────────────────────
    //  变量节点
    // ────────────────────────────────────────────────────────────────────────────

    // children[0]: 值表达式
    struct VarAssignNode : AIRNode {
        std::string name;
        VarAssignNode() : AIRNode(AIRNodeType::VarAssign) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string name) {
            auto n = std::make_unique<VarAssignNode>();
            n->name = std::move(name);
            return n;
        }
    };

    // children[0]: 初始值表达式（可选）
    struct VarDeclareNode : AIRNode {
        std::string name;
        VarScope    scope = VarScope::Local;
        VarDeclareNode() : AIRNode(AIRNodeType::VarDeclare) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string name,
                VarScope scope = VarScope::Local) {
            auto n = std::make_unique<VarDeclareNode>();
            n->name = std::move(name);
            n->scope = scope;
            return n;
        }
        bool hasInitValue() const { return !children.empty(); }
    };


    // ────────────────────────────────────────────────────────────────────────────
    //  键盘动作节点
    // ────────────────────────────────────────────────────────────────────────────

    // 对应：罗技 PressKey / AHK Send "{Key down}"
    struct KeyDownNode : AIRNode {
        AIRKey      key = AIRKey::KEY_UNKNOWN;
        std::string rawKey;
        KeyDownNode() : AIRNode(AIRNodeType::KeyDown) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(AIRKey key, const std::string& keyStr = "") {
            auto n = std::make_unique<KeyDownNode>();
            n->key = key;
            // 如果 key 未知，则记录原始字符串
            if (key == AIRKey::KEY_UNKNOWN) {
                n->rawKey = keyStr;
            }
            return n; // 这里会自动转换为 AIRNodePtr
        }
    };

    // 对应：罗技 ReleaseKey / AHK Send "{Key up}"
    struct KeyUpNode : AIRNode {
        AIRKey      key = AIRKey::KEY_UNKNOWN;
        std::string rawKey;
        KeyUpNode() : AIRNode(AIRNodeType::KeyUp) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(AIRKey key, std::string rawKey = {}) {
            auto n = std::make_unique<KeyUpNode>();
            n->key = key;
            n->rawKey = std::move(rawKey);
            return n;
        }
    };

    // 对应：罗技 PressAndReleaseKey / AHK Send "a"
    struct KeyTapNode : AIRNode {
        AIRKey      key = AIRKey::KEY_UNKNOWN;
        std::string rawKey;
        int         count = 1;
        int         holdMs = 0;
        KeyTapNode() : AIRNode(AIRNodeType::KeyTap) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(AIRKey key, int count = 1,
                int holdMs = 0, std::string rawKey = {}) {
            auto n = std::make_unique<KeyTapNode>();
            n->key = key;
            n->count = count;
            n->holdMs = holdMs;
            n->rawKey = std::move(rawKey);
            return n;
        }
    };

    // 对应：AHK SendText / 按键精灵 SayString
    struct KeySendStringNode : AIRNode {
        std::string text;
        KeySendStringNode() : AIRNode(AIRNodeType::KeySendString) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string text) {
            auto n = std::make_unique<KeySendStringNode>();
            n->text = std::move(text);
            return n;
        }
    };


    // ────────────────────────────────────────────────────────────────────────────
    //  鼠标动作节点
    // ────────────────────────────────────────────────────────────────────────────

    // children[0]: X 轴位移/坐标表达式
    // children[1]: Y 轴位移/坐标表达式
    // 对应：罗技 MoveMouseRelative / AHK MouseMove
    struct MouseMoveNode : AIRNode {
        CoordType coordType = CoordType::CoordRelative;
        MouseMoveNode() : AIRNode(AIRNodeType::MouseMove) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(CoordType ct = CoordType::CoordRelative) {
            auto n = std::make_unique<MouseMoveNode>();
            n->coordType = ct;
            return n;
        }
        // 快捷构造（实现在 §6）
        static AIRNodePtr relative(int dx, int dy);
    };

    // children[0]: X 坐标（可选）
    // children[1]: Y 坐标（可选）
    struct MouseClickNode : AIRNode {
        AIRKey key = AIRKey::KEY_MOUSE_LEFT;
        int    count = 1;
        int    holdMs = 0;
        MouseClickNode() : AIRNode(AIRNodeType::MouseClick) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(AIRKey key = AIRKey::KEY_MOUSE_LEFT,
                int count = 1) {
            auto n = std::make_unique<MouseClickNode>();
            n->key = key;
            n->count = count;
            return n;
        }
    };

    // 对应：罗技 PressMouseButton / AHK Click "down"
    struct MouseDownNode : AIRNode {
        AIRKey key = AIRKey::KEY_MOUSE_LEFT;
        MouseDownNode() : AIRNode(AIRNodeType::MouseDown) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(AIRKey key = AIRKey::KEY_MOUSE_LEFT) {
            auto n = std::make_unique<MouseDownNode>();
            n->key = key;
            return n;
        }
    };

    // 对应：罗技 ReleaseMouseButton / AHK Click "up"
    struct MouseUpNode : AIRNode {
        AIRKey key = AIRKey::KEY_MOUSE_LEFT;
        MouseUpNode() : AIRNode(AIRNodeType::MouseUp) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(AIRKey key = AIRKey::KEY_MOUSE_LEFT) {
            auto n = std::make_unique<MouseUpNode>();
            n->key = key;
            return n;
        }
    };

    // children[0]: 滚动格数表达式（可选，默认 1）
    // 对应：罗技 MoveMouseWheel / AHK Click "WheelUp"
    struct MouseScrollNode : AIRNode {
        ScrollDir dir = ScrollDir::Up;
        MouseScrollNode() : AIRNode(AIRNodeType::MouseScroll) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(ScrollDir dir = ScrollDir::Up) {
            auto n = std::make_unique<MouseScrollNode>();
            n->dir = dir;
            return n;
        }
    };


    // ────────────────────────────────────────────────────────────────────────────
    //  时间节点
    // ────────────────────────────────────────────────────────────────────────────

    // children[0]: 毫秒数表达式
    //    固定延迟：ExprNumberNode(50)
    //    随机延迟：ExprRandomRangeNode(20, 30)
    // 对应：罗技 Sleep / AHK Sleep
    struct SleepNode : AIRNode {
        SleepNode() : AIRNode(AIRNodeType::Sleep) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(int ms);            // 固定延迟（实现在 §6）
        static AIRNodePtr makeRandom(int min, int max); // 随机延迟（实现在 §6）
    };


    // ────────────────────────────────────────────────────────────────────────────
    //  查询节点
    // ────────────────────────────────────────────────────────────────────────────

    // 查询某键当前是否被按住（返回布尔值，用作条件表达式）
    // 对应：罗技 IsMouseButtonPressed / AHK GetKeyState("Key","P")
    struct QueryKeyStateNode : AIRNode {
        AIRKey key = AIRKey::KEY_UNKNOWN;
        QueryKeyStateNode() : AIRNode(AIRNodeType::QueryKeyState) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(AIRKey key) {
            auto n = std::make_unique<QueryKeyStateNode>();
            n->key = key;
            return n;
        }
    };


    // ────────────────────────────────────────────────────────────────────────────
    //  表达式节点
    // ────────────────────────────────────────────────────────────────────────────

    struct ExprNumberNode : AIRNode {
        double value = 0.0;
        ExprNumberNode() : AIRNode(AIRNodeType::ExprNumber) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(double val) {
            auto n = std::make_unique<ExprNumberNode>();
            n->value = val;
            return n;
        }
    };

    struct ExprStringNode : AIRNode {
        std::string value;
        ExprStringNode() : AIRNode(AIRNodeType::ExprString) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string val) {
            auto n = std::make_unique<ExprStringNode>();
            n->value = std::move(val);
            return n;
        }
    };

    struct ExprBoolNode : AIRNode {
        bool value = false;
        ExprBoolNode() : AIRNode(AIRNodeType::ExprBool) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(bool val) {
            auto n = std::make_unique<ExprBoolNode>();
            n->value = val;
            return n;
        }
    };

    struct ExprVarNode : AIRNode {
        std::string name;
        ExprVarNode() : AIRNode(AIRNodeType::ExprVar) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string name) {
            auto n = std::make_unique<ExprVarNode>();
            n->name = std::move(name);
            return n;
        }
    };

    // children: 实参表达式列表
    struct ExprCallNode : AIRNode {
        std::string name;
        ExprCallNode() : AIRNode(AIRNodeType::ExprCall) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string name) {
            auto n = std::make_unique<ExprCallNode>();
            n->name = std::move(name);
            return n;
        }
    };

    // children[0]: 左操作数
    // children[1]: 右操作数
    struct ExprBinopNode : AIRNode {
        BinOp op = BinOp::Add;
        ExprBinopNode() : AIRNode(AIRNodeType::ExprBinop) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(BinOp op) {
            auto n = std::make_unique<ExprBinopNode>();
            n->op = op;
            return n;
        }
    };

    // children[0]: 操作数
    struct ExprUnopNode : AIRNode {
        UnOp op = UnOp::OpNot;
        ExprUnopNode() : AIRNode(AIRNodeType::ExprUnop) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(UnOp op) {
            auto n = std::make_unique<ExprUnopNode>();
            n->op = op;
            return n;
        }
    };

    // children[0]: 条件
    // children[1]: 为真时的值
    // children[2]: 为假时的值
    struct ExprTernaryNode : AIRNode {
        ExprTernaryNode() : AIRNode(AIRNodeType::ExprTernary) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make() { return std::make_unique<ExprTernaryNode>(); }
    };

    // 随机整数范围 [min, max]
    // children[0]: 最小值表达式
    // children[1]: 最大值表达式
    // 主要用途：Sleep(ExprRandomRange(20,30)) 随机延迟防反作弊
    struct ExprRandomRangeNode : AIRNode {
        ExprRandomRangeNode() : AIRNode(AIRNodeType::ExprRandomRange) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(int minVal, int maxVal); // 实现在 §6
    };


    // ────────────────────────────────────────────────────────────────────────────
    //  元信息节点
    // ────────────────────────────────────────────────────────────────────────────

    struct CommentNode : AIRNode {
        std::string text;
        bool        isInline = false;
        CommentNode() : AIRNode(AIRNodeType::Comment) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string text, bool isInline = false) {
            auto n = std::make_unique<CommentNode>();
            n->text = std::move(text);
            n->isInline = isInline;
            return n;
        }
    };

    struct RawNode : AIRNode {
        std::string code;
        RawNode() : AIRNode(AIRNodeType::Raw) {}
        AIR_ACCEPT_IMPL
            static AIRNodePtr make(std::string code) {
            auto n = std::make_unique<RawNode>();
            n->code = std::move(code);
            return n;
        }
    };

#undef AIR_ACCEPT_IMPL


    // ============================================================================
    //  §6  延迟实现的工厂方法
    // ============================================================================

    inline AIRNodePtr MouseMoveNode::relative(int dx, int dy) {
        auto n = std::make_unique<MouseMoveNode>();
        n->coordType = CoordType::CoordRelative;
        n->children.push_back(ExprNumberNode::make(dx));
        n->children.push_back(ExprNumberNode::make(dy));
        return n;
    }

    inline AIRNodePtr SleepNode::make(int ms) {
        auto n = std::make_unique<SleepNode>();
        n->children.push_back(ExprNumberNode::make(ms));
        return n;
    }

    inline AIRNodePtr SleepNode::makeRandom(int minMs, int maxMs) {
        auto n = std::make_unique<SleepNode>();
        n->children.push_back(ExprRandomRangeNode::make(minMs, maxMs));
        return n;
    }

    inline AIRNodePtr ExprRandomRangeNode::make(int minVal, int maxVal) {
        auto n = std::make_unique<ExprRandomRangeNode>();
        n->children.push_back(ExprNumberNode::make(minVal));
        n->children.push_back(ExprNumberNode::make(maxVal));
        return n;
    }


    // ============================================================================
    //  §7  访问者接口  IAIRVisitor
    // ============================================================================

    class IAIRVisitor {
    public:
        virtual ~IAIRVisitor() = default;

        virtual void visit(ProgramNode& node) = 0;
        virtual void visit(SequenceNode& node) = 0;
        virtual void visit(FunctionDefNode& node) = 0;
        virtual void visit(FunctionCallNode& node) = 0;

        virtual void visit(TriggerNode& node) = 0;
        virtual void visit(HotkeyNode& node) = 0;

        virtual void visit(IfNode& node) = 0;
        virtual void visit(LoopCountNode& node) = 0;
        virtual void visit(LoopWhileNode& node) = 0;
        virtual void visit(LoopDoWhileNode& node) = 0;
        virtual void visit(LoopInfiniteNode& node) = 0;
        virtual void visit(BreakNode& node) = 0;
        virtual void visit(ContinueNode& node) = 0;
        virtual void visit(ReturnNode& node) = 0;
        virtual void visit(LabelNode& node) = 0;
        virtual void visit(GotoNode& node) = 0;

        virtual void visit(VarAssignNode& node) = 0;
        virtual void visit(VarDeclareNode& node) = 0;

        virtual void visit(KeyDownNode& node) = 0;
        virtual void visit(KeyUpNode& node) = 0;
        virtual void visit(KeyTapNode& node) = 0;
        virtual void visit(KeySendStringNode& node) = 0;

        virtual void visit(MouseMoveNode& node) = 0;
        virtual void visit(MouseClickNode& node) = 0;
        virtual void visit(MouseDownNode& node) = 0;
        virtual void visit(MouseUpNode& node) = 0;
        virtual void visit(MouseScrollNode& node) = 0;

        virtual void visit(SleepNode& node) = 0;

        virtual void visit(QueryKeyStateNode& node) = 0;

        virtual void visit(ExprNumberNode& node) = 0;
        virtual void visit(ExprStringNode& node) = 0;
        virtual void visit(ExprBoolNode& node) = 0;
        virtual void visit(ExprVarNode& node) = 0;
        virtual void visit(ExprCallNode& node) = 0;
        virtual void visit(ExprBinopNode& node) = 0;
        virtual void visit(ExprUnopNode& node) = 0;
        virtual void visit(ExprTernaryNode& node) = 0;
        virtual void visit(ExprRandomRangeNode& node) = 0;

        virtual void visit(CommentNode& node) = 0;
        virtual void visit(RawNode& node) = 0;
    };


    // ============================================================================
    //  §8  转换诊断  AIRDiagnostic
    // ============================================================================

    enum class DiagLevel {
        Info,
        Warning,
        Error,
    };

    struct AIRDiagnostic {
        DiagLevel   level;
        int         srcLine = 0;
        std::string message;
        std::string suggestion;
    };


    // ============================================================================
    //  §9  插件接口  IScriptPlugin
    // ============================================================================

    class IScriptPlugin {
    public:
        virtual ~IScriptPlugin() = default;

        virtual const char* GetFormatName()    const = 0;
        virtual const char* GetFileExtension() const = 0;
        virtual const char* GetPluginVersion() const = 0;

        virtual AIRNodePtr  Parse(
            const std::string& sourceText,
            std::vector<AIRDiagnostic>& outDiags) = 0;

        virtual std::string Generate(
            const AIRNode& root,
            std::vector<AIRDiagnostic>& outDiags) = 0;

        virtual bool CanParse()    const { return true; }
        virtual bool CanGenerate() const { return true; }
    };


    // ============================================================================
    //  §10 accept 成员函数具体实现 (修复 C2027)
    // ============================================================================
    // 必须在 IAIRVisitor 定义完整后才能调用其成员函数。

    inline void ProgramNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void SequenceNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void FunctionDefNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void FunctionCallNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void TriggerNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void HotkeyNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void IfNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void LoopCountNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void LoopWhileNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void LoopDoWhileNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void LoopInfiniteNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void BreakNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void ContinueNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void ReturnNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void LabelNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void GotoNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void VarAssignNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void VarDeclareNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void KeyDownNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void KeyUpNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void KeyTapNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void KeySendStringNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void MouseMoveNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void MouseClickNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void MouseDownNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void MouseUpNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void MouseScrollNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void SleepNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void QueryKeyStateNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void ExprNumberNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void ExprStringNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void ExprBoolNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void ExprVarNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void ExprCallNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void ExprBinopNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void ExprUnopNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void ExprTernaryNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void ExprRandomRangeNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void CommentNode::accept(IAIRVisitor& v) { v.visit(*this); }
    inline void RawNode::accept(IAIRVisitor& v) { v.visit(*this); }

} // namespace AIR
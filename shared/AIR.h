/*
 * AIR.h  —  Abstract Intermediate Representation  v0.4
 *
 * 游戏宏跨平台转换工具 —— 中间表示层定义
 *
 * ============================================================================
 * v0.4  简化前向声明（解决 v0.3 的繁琐问题）
 * ============================================================================
 *
 * v0.3 的问题：
 *   AIR_ACCEPT_IMPL 宏在类体内写 { v.visit(*this); }，
 *   但 IAIRVisitor 定义在所有节点类之后，编译器尚未看到 visit 的完整签名。
 *   你的修法（§10 那 32 行 inline 实现）是正确的，但每加一个节点就多一行。
 *
 * v0.4 的解法（调整顺序，消除根本原因）：
 *
 *   文件分三段：
 *   ┌────────────────────────────────────────────────────┐
 *   │ 第一段：枚举 + AIRNode 基类                         │
 *   └────────────────────────────────────────────────────┘
 *   ┌────────────────────────────────────────────────────┐
 *   │ 第二段：IAIRVisitor                                 │
 *   │  - 只需所有节点的【前向声明】（一行一个 struct）     │
 *   │  - 然后定义 IAIRVisitor（visit 参数只需前向声明）    │
 *   └────────────────────────────────────────────────────┘
 *   ┌────────────────────────────────────────────────────┐
 *   │ 第三段：派生节点类（此时 IAIRVisitor 已完整定义）    │
 *   │  - AIR_ACCEPT_IMPL 直接带函数体，无需 §10           │
 *   └────────────────────────────────────────────────────┘
 *
 *   结果：§10 整段删掉，AIR_ACCEPT_IMPL 恢复为带函数体的一行宏，
 *   新增节点只需在三处各加一行，不用再手写 inline 实现。
 *
 * ============================================================================
 * 命名冲突修复（延续 v0.3，不变）
 * ============================================================================
 *   RELATIVE/ABSOLUTE → CoordType::CoordRelative/CoordAbsolute
 *   MOD_xxx           → ModMask::None/Ctrl/Shift/Alt/Win  (enum class)
 *   MOD/AND/OR/NOT    → BinOp::OpMod/OpAnd/OpOr, UnOp::OpNot
 *
 * ============================================================================
 * 插件编译约定
 * ============================================================================
 *   1. 与主程序相同版本 Visual Studio
 *   2. /MD 或 /MDd（动态链接 CRT，禁止 /MT）
 *   3. /D UNICODE
 */

#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace AIR {

// ============================================================================
//  ★ 第一段：枚举 & AIRNode 基类
// ============================================================================

// §1  AIRKey ──────────────────────────────────────────────────────────────────
enum class AIRKey : uint16_t {
    KEY_MOUSE_LEFT, KEY_MOUSE_RIGHT, KEY_MOUSE_MIDDLE, KEY_MOUSE_X1, KEY_MOUSE_X2,
    KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F,KEY_G,KEY_H,KEY_I,KEY_J,KEY_K,KEY_L,KEY_M,
    KEY_N,KEY_O,KEY_P,KEY_Q,KEY_R,KEY_S,KEY_T,KEY_U,KEY_V,KEY_W,KEY_X,KEY_Y,KEY_Z,
    KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,
    KEY_NUMPAD0,KEY_NUMPAD1,KEY_NUMPAD2,KEY_NUMPAD3,KEY_NUMPAD4,
    KEY_NUMPAD5,KEY_NUMPAD6,KEY_NUMPAD7,KEY_NUMPAD8,KEY_NUMPAD9,
    KEY_NUMPAD_DOT,KEY_NUMPAD_ADD,KEY_NUMPAD_SUB,KEY_NUMPAD_MUL,
    KEY_NUMPAD_DIV,KEY_NUMPAD_ENTER,KEY_NUMLOCK,
    KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,KEY_F6,KEY_F7,KEY_F8,
    KEY_F9,KEY_F10,KEY_F11,KEY_F12,KEY_F13,KEY_F14,KEY_F15,KEY_F16,
    KEY_F17,KEY_F18,KEY_F19,KEY_F20,KEY_F21,KEY_F22,KEY_F23,KEY_F24,
    KEY_CTRL,KEY_LCTRL,KEY_RCTRL,KEY_SHIFT,KEY_LSHIFT,KEY_RSHIFT,
    KEY_ALT,KEY_LALT,KEY_RALT,KEY_WIN,KEY_LWIN,KEY_RWIN,
    KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,
    KEY_HOME,KEY_END,KEY_PGUP,KEY_PGDN,KEY_INSERT,KEY_DELETE,
    KEY_ENTER,KEY_BACKSPACE,KEY_TAB,KEY_SPACE,KEY_ESCAPE,
    KEY_CAPSLOCK,KEY_SCROLLLOCK,KEY_PAUSE,KEY_PRINTSCREEN,
    KEY_BACKTICK,KEY_MINUS,KEY_EQUALS,KEY_LBRACKET,KEY_RBRACKET,
    KEY_BACKSLASH,KEY_SEMICOLON,KEY_QUOTE,KEY_COMMA,KEY_DOT,KEY_SLASH,
    KEY_MEDIA_PLAY_PAUSE,KEY_MEDIA_STOP,KEY_MEDIA_NEXT,KEY_MEDIA_PREV,
    KEY_VOLUME_UP,KEY_VOLUME_DOWN,KEY_VOLUME_MUTE,
    KEY_UNKNOWN,
};

// §2  辅助枚举 ─────────────────────────────────────────────────────────────────

enum class ScrollDir  { Up, Down, Left, Right };

// CoordRelative/CoordAbsolute 前缀：回避 wingdi.h 的 #define RELATIVE 0 / ABSOLUTE 1
enum class CoordType  { CoordRelative, CoordAbsolute };

// ModMask (enum class)：回避 SDL2/GTK 的 MOD_SHIFT/MOD_ALT/MOD_CTRL 宏冲突
enum class ModMask : uint8_t { None=0x00, Ctrl=0x01, Shift=0x02, Alt=0x04, Win=0x08 };
inline ModMask  operator| (ModMask a, ModMask b) noexcept { return static_cast<ModMask>(static_cast<uint8_t>(a)|static_cast<uint8_t>(b)); }
inline ModMask  operator& (ModMask a, ModMask b) noexcept { return static_cast<ModMask>(static_cast<uint8_t>(a)&static_cast<uint8_t>(b)); }
inline ModMask& operator|=(ModMask& a, ModMask b) noexcept { a = a|b; return a; }
inline bool hasModifier(ModMask m, ModMask f) noexcept { return (static_cast<uint8_t>(m)&static_cast<uint8_t>(f))!=0; }

enum class TriggerEvent { KeyDown, KeyUp, MouseButtonDown, MouseButtonUp, MacroStart, MacroStop };

// OpMod/OpAnd/OpOr/OpNot 前缀：回避 <cmath>/<iso646.h> 宏冲突
enum class BinOp { Add,Sub,Mul,Div,OpMod,Pow, Eq,Neq,Lt,Gt,Lte,Gte, OpAnd,OpOr, Concat };
enum class UnOp  { OpNot, Neg };
enum class VarScope { Local, Global, Static };

// §3  AIRNodeType ─────────────────────────────────────────────────────────────
enum class AIRNodeType {
    Program, Sequence, FunctionDef, FunctionCall,
    Trigger, Hotkey,
    If, LoopCount, LoopWhile, LoopDoWhile, LoopInfinite,
    Break, Continue, Return, Label, Goto,
    VarAssign, VarDeclare,
    KeyDown, KeyUp, KeyTap, KeySendString,
    MouseMove, MouseClick, MouseDown, MouseUp, MouseScroll,
    Sleep, QueryKeyState,
    ExprNumber, ExprString, ExprBool, ExprVar, ExprCall,
    ExprBinop, ExprUnop, ExprTernary, ExprRandomRange,
    Comment, Raw,
};

// §4  AIRNode 基类 ─────────────────────────────────────────────────────────────
struct AIRNode;
using  AIRNodePtr = std::unique_ptr<AIRNode>;
class  IAIRVisitor; // 仅前向声明，第二段完整定义

struct AIRNode {
    AIRNodeType             type;
    std::vector<AIRNodePtr> children;
    int                     srcLine = 0;
    std::string             srcFile;

    explicit AIRNode(AIRNodeType t) : type(t) {}
    virtual ~AIRNode() = default;
    AIRNode(const AIRNode&)            = delete;
    AIRNode& operator=(const AIRNode&) = delete;
    AIRNode(AIRNode&&)                 = default;
    AIRNode& operator=(AIRNode&&)      = default;

    AIRNode& addChild(AIRNodePtr c) { children.push_back(std::move(c)); return *this; }
    virtual void accept(IAIRVisitor& v) = 0;
};


// ============================================================================
//  ★ 第二段：IAIRVisitor
//
//  visit() 的参数只需要前向声明（不需要完整类型），所以把 IAIRVisitor 放在
//  节点类定义之前，节点类内的 accept 就能直接写 { v.visit(*this); }。
// ============================================================================

// 所有节点类的前向声明（仅为 IAIRVisitor 的 visit 参数列表服务）
struct ProgramNode; struct SequenceNode; struct FunctionDefNode; struct FunctionCallNode;
struct TriggerNode; struct HotkeyNode;
struct IfNode; struct LoopCountNode; struct LoopWhileNode; struct LoopDoWhileNode;
struct LoopInfiniteNode; struct BreakNode; struct ContinueNode; struct ReturnNode;
struct LabelNode; struct GotoNode; struct VarAssignNode; struct VarDeclareNode;
struct KeyDownNode; struct KeyUpNode; struct KeyTapNode; struct KeySendStringNode;
struct MouseMoveNode; struct MouseClickNode; struct MouseDownNode; struct MouseUpNode;
struct MouseScrollNode; struct SleepNode; struct QueryKeyStateNode;
struct ExprNumberNode; struct ExprStringNode; struct ExprBoolNode; struct ExprVarNode;
struct ExprCallNode; struct ExprBinopNode; struct ExprUnopNode; struct ExprTernaryNode;
struct ExprRandomRangeNode; struct CommentNode; struct RawNode;

class IAIRVisitor {
public:
    virtual ~IAIRVisitor() = default;
    virtual void visit(ProgramNode&)        = 0;
    virtual void visit(SequenceNode&)       = 0;
    virtual void visit(FunctionDefNode&)    = 0;
    virtual void visit(FunctionCallNode&)   = 0;
    virtual void visit(TriggerNode&)        = 0;
    virtual void visit(HotkeyNode&)         = 0;
    virtual void visit(IfNode&)             = 0;
    virtual void visit(LoopCountNode&)      = 0;
    virtual void visit(LoopWhileNode&)      = 0;
    virtual void visit(LoopDoWhileNode&)    = 0;
    virtual void visit(LoopInfiniteNode&)   = 0;
    virtual void visit(BreakNode&)          = 0;
    virtual void visit(ContinueNode&)       = 0;
    virtual void visit(ReturnNode&)         = 0;
    virtual void visit(LabelNode&)          = 0;
    virtual void visit(GotoNode&)           = 0;
    virtual void visit(VarAssignNode&)      = 0;
    virtual void visit(VarDeclareNode&)     = 0;
    virtual void visit(KeyDownNode&)        = 0;
    virtual void visit(KeyUpNode&)          = 0;
    virtual void visit(KeyTapNode&)         = 0;
    virtual void visit(KeySendStringNode&)  = 0;
    virtual void visit(MouseMoveNode&)      = 0;
    virtual void visit(MouseClickNode&)     = 0;
    virtual void visit(MouseDownNode&)      = 0;
    virtual void visit(MouseUpNode&)        = 0;
    virtual void visit(MouseScrollNode&)    = 0;
    virtual void visit(SleepNode&)          = 0;
    virtual void visit(QueryKeyStateNode&)  = 0;
    virtual void visit(ExprNumberNode&)     = 0;
    virtual void visit(ExprStringNode&)     = 0;
    virtual void visit(ExprBoolNode&)       = 0;
    virtual void visit(ExprVarNode&)        = 0;
    virtual void visit(ExprCallNode&)       = 0;
    virtual void visit(ExprBinopNode&)      = 0;
    virtual void visit(ExprUnopNode&)       = 0;
    virtual void visit(ExprTernaryNode&)    = 0;
    virtual void visit(ExprRandomRangeNode&)= 0;
    virtual void visit(CommentNode&)        = 0;
    virtual void visit(RawNode&)            = 0;
};


// ============================================================================
//  ★ 第三段：派生节点类
//
//  IAIRVisitor 已完整定义，accept() 直接写函数体，§10 不再需要。
// ============================================================================

// 宏：在类体内生成 accept() 实现（IAIRVisitor 已完整定义，可直接调用 v.visit）
#define AIR_ACCEPT void accept(IAIRVisitor& v) override { v.visit(*this); }

// ── 程序结构 ──────────────────────────────────────────────────────────────────

struct ProgramNode : AIRNode {
    ProgramNode() : AIRNode(AIRNodeType::Program) {}
    AIR_ACCEPT
    static AIRNodePtr make() { return std::make_unique<ProgramNode>(); }
};

struct SequenceNode : AIRNode {
    SequenceNode() : AIRNode(AIRNodeType::Sequence) {}
    AIR_ACCEPT
    static AIRNodePtr make() { return std::make_unique<SequenceNode>(); }
};

struct FunctionDefNode : AIRNode {
    std::string name;
    std::vector<std::string> params;
    FunctionDefNode() : AIRNode(AIRNodeType::FunctionDef) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string name, std::vector<std::string> params = {}) {
        auto n = std::make_unique<FunctionDefNode>();
        n->name = std::move(name); n->params = std::move(params); return n;
    }
};

struct FunctionCallNode : AIRNode {
    std::string name;
    FunctionCallNode() : AIRNode(AIRNodeType::FunctionCall) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string name) {
        auto n = std::make_unique<FunctionCallNode>(); n->name = std::move(name); return n;
    }
};

// ── 触发器 / 热键 ─────────────────────────────────────────────────────────────

// children[0]: SequenceNode（触发时执行的动作）
struct TriggerNode : AIRNode {
    TriggerEvent triggerEvent = TriggerEvent::MouseButtonDown;
    AIRKey       key          = AIRKey::KEY_MOUSE_LEFT;
    ModMask      modifiers    = ModMask::None;
    TriggerNode() : AIRNode(AIRNodeType::Trigger) {}
    AIR_ACCEPT
    static AIRNodePtr make(TriggerEvent evt, AIRKey key, ModMask mods = ModMask::None) {
        auto n = std::make_unique<TriggerNode>();
        n->triggerEvent = evt; n->key = key; n->modifiers = mods; return n;
    }
};

// children[0]: SequenceNode
struct HotkeyNode : AIRNode {
    AIRKey  key         = AIRKey::KEY_UNKNOWN;
    ModMask modifiers   = ModMask::None;
    bool    passthrough = false;
    HotkeyNode() : AIRNode(AIRNodeType::Hotkey) {}
    AIR_ACCEPT
    static AIRNodePtr make(AIRKey key, ModMask mods = ModMask::None, bool passthrough = false) {
        auto n = std::make_unique<HotkeyNode>();
        n->key = key; n->modifiers = mods; n->passthrough = passthrough; return n;
    }
};

// ── 控制流 ────────────────────────────────────────────────────────────────────

// children[0]:条件  children[1]:then块  children[2]:else块（可选）
struct IfNode : AIRNode {
    IfNode() : AIRNode(AIRNodeType::If) {}
    AIR_ACCEPT
    static AIRNodePtr make() { return std::make_unique<IfNode>(); }
    bool hasElse() const { return children.size() >= 3; }
};

// children[0]:次数表达式  children[1]:循环体
struct LoopCountNode : AIRNode {
    LoopCountNode() : AIRNode(AIRNodeType::LoopCount) {}
    AIR_ACCEPT
    static AIRNodePtr make() { return std::make_unique<LoopCountNode>(); }
};

// children[0]:条件（为真继续）  children[1]:循环体
struct LoopWhileNode : AIRNode {
    LoopWhileNode() : AIRNode(AIRNodeType::LoopWhile) {}
    AIR_ACCEPT
    static AIRNodePtr make() { return std::make_unique<LoopWhileNode>(); }
};

// children[0]:循环体（至少执行一次）  children[1]:退出条件（为真时退出，until语义）
struct LoopDoWhileNode : AIRNode {
    LoopDoWhileNode() : AIRNode(AIRNodeType::LoopDoWhile) {}
    AIR_ACCEPT
    static AIRNodePtr make() { return std::make_unique<LoopDoWhileNode>(); }
};

// children[0]:循环体
struct LoopInfiniteNode : AIRNode {
    LoopInfiniteNode() : AIRNode(AIRNodeType::LoopInfinite) {}
    AIR_ACCEPT
    static AIRNodePtr make() { return std::make_unique<LoopInfiniteNode>(); }
};

struct BreakNode    : AIRNode { BreakNode()   :AIRNode(AIRNodeType::Break)    {} AIR_ACCEPT static AIRNodePtr make(){return std::make_unique<BreakNode>();}    };
struct ContinueNode : AIRNode { ContinueNode():AIRNode(AIRNodeType::Continue) {} AIR_ACCEPT static AIRNodePtr make(){return std::make_unique<ContinueNode>();} };

// children[0]:返回值（可选）
struct ReturnNode : AIRNode {
    ReturnNode() : AIRNode(AIRNodeType::Return) {}
    AIR_ACCEPT
    static AIRNodePtr make() { return std::make_unique<ReturnNode>(); }
    bool hasValue() const { return !children.empty(); }
};

struct LabelNode : AIRNode {
    std::string name;
    LabelNode() : AIRNode(AIRNodeType::Label) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string name) {
        auto n = std::make_unique<LabelNode>(); n->name = std::move(name); return n;
    }
};

struct GotoNode : AIRNode {
    std::string target;
    GotoNode() : AIRNode(AIRNodeType::Goto) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string target) {
        auto n = std::make_unique<GotoNode>(); n->target = std::move(target); return n;
    }
};

// ── 变量 ──────────────────────────────────────────────────────────────────────

// children[0]:值表达式
struct VarAssignNode : AIRNode {
    std::string name;
    VarAssignNode() : AIRNode(AIRNodeType::VarAssign) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string name) {
        auto n = std::make_unique<VarAssignNode>(); n->name = std::move(name); return n;
    }
};

// children[0]:初始值（可选）
struct VarDeclareNode : AIRNode {
    std::string name;
    VarScope    scope = VarScope::Local;
    VarDeclareNode() : AIRNode(AIRNodeType::VarDeclare) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string name, VarScope scope = VarScope::Local) {
        auto n = std::make_unique<VarDeclareNode>();
        n->name = std::move(name); n->scope = scope; return n;
    }
    bool hasInitValue() const { return !children.empty(); }
};

// ── 键盘动作 ──────────────────────────────────────────────────────────────────

struct KeyDownNode : AIRNode {
    AIRKey      key = AIRKey::KEY_UNKNOWN;
    std::string rawKey;
    KeyDownNode() : AIRNode(AIRNodeType::KeyDown) {}
    AIR_ACCEPT
        static AIRNodePtr make(AIRKey key, std::string rawKey = {}) {
        auto n = std::make_unique<KeyDownNode>();
        n->key = key;
        // 如果 key 未知，则记录原始字符串
        if (key == AIRKey::KEY_UNKNOWN) {
            n->rawKey = std::move(rawKey);
        }
        return n; // 这里会自动转换为 AIRNodePtr
    }
};

/*struct KeyDownNode : AIRNode {
    AIRKey      key    = AIRKey::KEY_UNKNOWN;
    std::string rawKey; // key==KEY_UNKNOWN 时保留原始键名
    KeyDownNode() : AIRNode(AIRNodeType::KeyDown) {}
    AIR_ACCEPT
    static AIRNodePtr make(AIRKey key, std::string rawKey = {}) {
        auto n = std::make_unique<KeyDownNode>(); n->key = key; n->rawKey = std::move(rawKey); return n;
    }
};*/

struct KeyUpNode : AIRNode {
    AIRKey      key    = AIRKey::KEY_UNKNOWN;
    std::string rawKey;
    KeyUpNode() : AIRNode(AIRNodeType::KeyUp) {}
    AIR_ACCEPT
    static AIRNodePtr make(AIRKey key, std::string rawKey = {}) {
        auto n = std::make_unique<KeyUpNode>(); n->key = key; n->rawKey = std::move(rawKey); return n;
    }
};

struct KeyTapNode : AIRNode {
    AIRKey      key    = AIRKey::KEY_UNKNOWN;
    std::string rawKey;
    int         count  = 1;
    int         holdMs = 0;
    KeyTapNode() : AIRNode(AIRNodeType::KeyTap) {}
    AIR_ACCEPT
    static AIRNodePtr make(AIRKey key, int count = 1, int holdMs = 0, std::string rawKey = {}) {
        auto n = std::make_unique<KeyTapNode>();
        n->key = key; n->count = count; n->holdMs = holdMs; n->rawKey = std::move(rawKey); return n;
    }
};

struct KeySendStringNode : AIRNode {
    std::string text;
    KeySendStringNode() : AIRNode(AIRNodeType::KeySendString) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string text) {
        auto n = std::make_unique<KeySendStringNode>(); n->text = std::move(text); return n;
    }
};

// ── 鼠标动作 ──────────────────────────────────────────────────────────────────

// children[0]:X表达式  children[1]:Y表达式
struct MouseMoveNode : AIRNode {
    CoordType coordType = CoordType::CoordRelative;
    MouseMoveNode() : AIRNode(AIRNodeType::MouseMove) {}
    AIR_ACCEPT
    static AIRNodePtr make(CoordType ct = CoordType::CoordRelative) {
        auto n = std::make_unique<MouseMoveNode>(); n->coordType = ct; return n;
    }
    static AIRNodePtr relative(int dx, int dy); // 实现在文件末尾
};

// children[0/1]:X/Y坐标（可选）
struct MouseClickNode : AIRNode {
    AIRKey key    = AIRKey::KEY_MOUSE_LEFT;
    int    count  = 1;
    int    holdMs = 0;
    MouseClickNode() : AIRNode(AIRNodeType::MouseClick) {}
    AIR_ACCEPT
    static AIRNodePtr make(AIRKey key = AIRKey::KEY_MOUSE_LEFT, int count = 1) {
        auto n = std::make_unique<MouseClickNode>(); n->key = key; n->count = count; return n;
    }
};

struct MouseDownNode : AIRNode {
    AIRKey key = AIRKey::KEY_MOUSE_LEFT;
    MouseDownNode() : AIRNode(AIRNodeType::MouseDown) {}
    AIR_ACCEPT
    static AIRNodePtr make(AIRKey key = AIRKey::KEY_MOUSE_LEFT) {
        auto n = std::make_unique<MouseDownNode>(); n->key = key; return n;
    }
};

struct MouseUpNode : AIRNode {
    AIRKey key = AIRKey::KEY_MOUSE_LEFT;
    MouseUpNode() : AIRNode(AIRNodeType::MouseUp) {}
    AIR_ACCEPT
    static AIRNodePtr make(AIRKey key = AIRKey::KEY_MOUSE_LEFT) {
        auto n = std::make_unique<MouseUpNode>(); n->key = key; return n;
    }
};

// children[0]:滚动格数（可选，默认1）
struct MouseScrollNode : AIRNode {
    ScrollDir dir = ScrollDir::Up;
    MouseScrollNode() : AIRNode(AIRNodeType::MouseScroll) {}
    AIR_ACCEPT
    static AIRNodePtr make(ScrollDir dir = ScrollDir::Up) {
        auto n = std::make_unique<MouseScrollNode>(); n->dir = dir; return n;
    }
};

// ── 时间 ──────────────────────────────────────────────────────────────────────

// children[0]:毫秒数（ExprNumberNode 或 ExprRandomRangeNode）
struct SleepNode : AIRNode {
    SleepNode() : AIRNode(AIRNodeType::Sleep) {}
    AIR_ACCEPT
    static AIRNodePtr make(int ms);                 // 实现在文件末尾
    static AIRNodePtr makeRandom(int min, int max); // 实现在文件末尾
};

// ── 查询 ──────────────────────────────────────────────────────────────────────

struct QueryKeyStateNode : AIRNode {
    AIRKey key = AIRKey::KEY_UNKNOWN;
    QueryKeyStateNode() : AIRNode(AIRNodeType::QueryKeyState) {}
    AIR_ACCEPT
    static AIRNodePtr make(AIRKey key) {
        auto n = std::make_unique<QueryKeyStateNode>(); n->key = key; return n;
    }
};

// ── 表达式 ────────────────────────────────────────────────────────────────────

struct ExprNumberNode : AIRNode {
    double value = 0.0;
    ExprNumberNode() : AIRNode(AIRNodeType::ExprNumber) {}
    AIR_ACCEPT
    static AIRNodePtr make(double val) {
        auto n = std::make_unique<ExprNumberNode>(); n->value = val; return n;
    }
};

struct ExprStringNode : AIRNode {
    std::string value;
    ExprStringNode() : AIRNode(AIRNodeType::ExprString) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string val) {
        auto n = std::make_unique<ExprStringNode>(); n->value = std::move(val); return n;
    }
};

struct ExprBoolNode : AIRNode {
    bool value = false;
    ExprBoolNode() : AIRNode(AIRNodeType::ExprBool) {}
    AIR_ACCEPT
    static AIRNodePtr make(bool val) {
        auto n = std::make_unique<ExprBoolNode>(); n->value = val; return n;
    }
};

struct ExprVarNode : AIRNode {
    std::string name;
    ExprVarNode() : AIRNode(AIRNodeType::ExprVar) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string name) {
        auto n = std::make_unique<ExprVarNode>(); n->name = std::move(name); return n;
    }
};

// children:实参列表
struct ExprCallNode : AIRNode {
    std::string name;
    ExprCallNode() : AIRNode(AIRNodeType::ExprCall) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string name) {
        auto n = std::make_unique<ExprCallNode>(); n->name = std::move(name); return n;
    }
};

// children[0]:左  children[1]:右
struct ExprBinopNode : AIRNode {
    BinOp op = BinOp::Add;
    ExprBinopNode() : AIRNode(AIRNodeType::ExprBinop) {}
    AIR_ACCEPT
    static AIRNodePtr make(BinOp op) {
        auto n = std::make_unique<ExprBinopNode>(); n->op = op; return n;
    }
};

// children[0]:操作数
struct ExprUnopNode : AIRNode {
    UnOp op = UnOp::OpNot;
    ExprUnopNode() : AIRNode(AIRNodeType::ExprUnop) {}
    AIR_ACCEPT
    static AIRNodePtr make(UnOp op) {
        auto n = std::make_unique<ExprUnopNode>(); n->op = op; return n;
    }
};

// children[0]:条件  children[1]:真值  children[2]:假值
struct ExprTernaryNode : AIRNode {
    ExprTernaryNode() : AIRNode(AIRNodeType::ExprTernary) {}
    AIR_ACCEPT
    static AIRNodePtr make() { return std::make_unique<ExprTernaryNode>(); }
};

// children[0]:min  children[1]:max  ← 随机延迟 Sleep(Random(20,30))
struct ExprRandomRangeNode : AIRNode {
    ExprRandomRangeNode() : AIRNode(AIRNodeType::ExprRandomRange) {}
    AIR_ACCEPT
    static AIRNodePtr make(int minVal, int maxVal); // 实现在文件末尾
};

// ── 元信息 ────────────────────────────────────────────────────────────────────

struct CommentNode : AIRNode {
    std::string text;
    bool        isInline = false;
    CommentNode() : AIRNode(AIRNodeType::Comment) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string text, bool isInline = false) {
        auto n = std::make_unique<CommentNode>(); n->text = std::move(text); n->isInline = isInline; return n;
    }
};

struct RawNode : AIRNode {
    std::string code;
    RawNode() : AIRNode(AIRNodeType::Raw) {}
    AIR_ACCEPT
    static AIRNodePtr make(std::string code) {
        auto n = std::make_unique<RawNode>(); n->code = std::move(code); return n;
    }
};

#undef AIR_ACCEPT


// ============================================================================
//  延迟工厂方法（依赖 ExprNumberNode / ExprRandomRangeNode，须在其定义之后）
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
//  §8  转换诊断 & 插件接口
// ============================================================================

enum class DiagLevel { Info, Warning, Error };

struct AIRDiagnostic {
    DiagLevel   level;
    int         srcLine = 0;
    std::string message;
    std::string suggestion;
};

class IScriptPlugin {
public:
    virtual ~IScriptPlugin() = default;
    virtual const char* GetFormatName()    const = 0;
    virtual const char* GetFileExtension() const = 0;
    virtual const char* GetPluginVersion() const = 0;
    virtual AIRNodePtr  Parse(const std::string& sourceText,
                              std::vector<AIRDiagnostic>& outDiags) = 0;
    virtual std::string Generate(const AIRNode& root,
                                 std::vector<AIRDiagnostic>& outDiags) = 0;
    virtual bool CanParse()    const { return true; }
    virtual bool CanGenerate() const { return true; }
    virtual const char* GetDemoCode() const { return ""; }
};

} // namespace AIR

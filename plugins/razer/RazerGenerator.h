/*
 * RazerGenerator.h  —  AIR 树 → 雷蛇 XML 宏生成器
 *
 * 职责：将 AIR 树转换回雷蛇 Synapse 可导入的 XML 格式。
 *
 * 支持的 AIR 节点 → XML 映射：
 *   SleepNode           → MacroEvent Type=6（纯延迟）
 *   MouseMoveNode       → MacroEvent Type=3 + MouseMovementEvent 序列
 *   KeyDownNode         → MacroEvent Type=1 + KeyboardEvent
 *   KeyUpNode           → MacroEvent Type=2 + KeyboardEvent
 *   MouseDownNode       → MacroEvent Type=4 + MouseButtonEvent
 *   MouseUpNode         → MacroEvent Type=5 + MouseButtonEvent
 *   KeyTapNode          → Type=1 + Type=2（分拆为按下+松开）
 *   MouseClickNode      → Type=4 + Type=5
 *   SequenceNode        → 展开为多个 MacroEvent
 *   CommentNode         → XML 注释 <!-- ... -->
 *   其余节点            → XML 注释警告
 *
 * 坐标说明：
 *   雷蛇格式只支持绝对坐标（Type=3）。
 *   若 AIR 中有相对坐标 MouseMoveNode，生成器会输出警告，
 *   并以相对偏移量作为绝对坐标生成（语义有损，用户需手动调整）。
 *
 * 连续 MouseMoveNode 合并：
 *   将相邻的 MouseMoveNode 合并进同一个 MacroEvent Type=3 的
 *   MouseMovement 序列，还原为雷蛇的原始格式。
 */

#pragma once

#include "AIR.h"
#include "RazerFormat.h"
#include <string>
#include <vector>
#include <sstream>

// ── 生成器内部用的"扁平事件"结构 ───────────────────────────────────────────
// AIR 树层次结构不直接对应雷蛇的线性 MacroEvent 列表，
// 先把 AIR 树展开为扁平事件列表，再批量生成 XML。

struct FlatEvent {
    enum class Kind {
        Sleep,          // 纯延迟
        KeyDown,        // 键盘按下
        KeyUp,          // 键盘松开
        MouseDown,      // 鼠标按下
        MouseUp,        // 鼠标松开
        MouseMove,      // 鼠标移动（绝对坐标）
        Comment,        // XML 注释
        Warning,        // 不支持的节点警告
    };

    Kind        kind    = Kind::Warning;
    int         delay   = 0;     // 毫秒
    AIR::AIRKey key     = AIR::AIRKey::KEY_UNKNOWN;
    std::string rawKey;          // KEY_UNKNOWN 时的原始键名
    int         x       = 0;    // MouseMove 用
    int         y       = 0;
    std::string text;            // Comment / Warning 文本
};

class RazerGenerator : public AIR::IAIRVisitor {
public:
    explicit RazerGenerator(std::vector<AIR::AIRDiagnostic>& diags,
                            RazerSynapseVersion version = RazerSynapseVersion::Synapse4);

    // 获取生成的 XML 字符串
    std::string result() const;

    // ── IAIRVisitor ─────────────────────────────────────────────────────────
    void visit(AIR::ProgramNode&      node) override;
    void visit(AIR::SequenceNode&     node) override;
    void visit(AIR::FunctionDefNode&  node) override;
    void visit(AIR::FunctionCallNode& node) override;
    void visit(AIR::TriggerNode&      node) override;
    void visit(AIR::HotkeyNode&       node) override;
    void visit(AIR::IfNode&           node) override;
    void visit(AIR::LoopCountNode&    node) override;
    void visit(AIR::LoopWhileNode&    node) override;
    void visit(AIR::LoopDoWhileNode&  node) override;
    void visit(AIR::LoopInfiniteNode& node) override;
    void visit(AIR::BreakNode&        node) override;
    void visit(AIR::ContinueNode&     node) override;
    void visit(AIR::ReturnNode&       node) override;
    void visit(AIR::LabelNode&        node) override;
    void visit(AIR::GotoNode&         node) override;
    void visit(AIR::VarAssignNode&    node) override;
    void visit(AIR::VarDeclareNode&   node) override;
    void visit(AIR::KeyDownNode&      node) override;
    void visit(AIR::KeyUpNode&        node) override;
    void visit(AIR::KeyTapNode&       node) override;
    void visit(AIR::KeySendStringNode&node) override;
    void visit(AIR::MouseMoveNode&    node) override;
    void visit(AIR::MouseClickNode&   node) override;
    void visit(AIR::MouseDownNode&    node) override;
    void visit(AIR::MouseUpNode&      node) override;
    void visit(AIR::MouseScrollNode&  node) override;
    void visit(AIR::SleepNode&        node) override;
    void visit(AIR::QueryKeyStateNode&node) override;
    void visit(AIR::ExprNumberNode&   node) override;
    void visit(AIR::ExprStringNode&   node) override;
    void visit(AIR::ExprBoolNode&     node) override;
    void visit(AIR::ExprVarNode&      node) override;
    void visit(AIR::ExprCallNode&     node) override;
    void visit(AIR::ExprBinopNode&    node) override;
    void visit(AIR::ExprUnopNode&     node) override;
    void visit(AIR::ExprTernaryNode&  node) override;
    void visit(AIR::ExprRandomRangeNode& node) override;
    void visit(AIR::CommentNode&      node) override;
    void visit(AIR::RawNode&          node) override;

private:
    // 收集扁平事件
    void pushEvent(FlatEvent ev) { m_events.push_back(std::move(ev)); }
    void pushWarning(const std::string& msg, int line = 0);

    // 从表达式节点提取整数值（用于坐标/延迟）
    int evalIntExpr(AIR::AIRNode* node, int defaultVal = 0);

    // 将扁平事件列表生成 XML
    std::string buildXml(const std::string& macroName) const;

    // 生成单个 MacroEvent XML 块
    // moveGroup：连续 MouseMove 事件合并为一个 Type=3
    std::string buildMacroEventXml(size_t& i) const;
    std::string buildMouseMovementXml(size_t& i) const;

    // XML 转义
    static std::string xmlEscape(const std::string& s);

    std::vector<FlatEvent>           m_events;
    std::string                      m_macroName;
    std::string                      m_macroGuid;
    std::vector<AIR::AIRDiagnostic>& m_diags;
    RazerSynapseVersion              m_version;
};

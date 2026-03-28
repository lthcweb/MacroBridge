/*
 * AhkGenerator.h  —  AIR 树 → AHK v2 代码生成器
 *
 * 实现 IAIRVisitor，遍历 AIR 树输出 AHK v2 格式代码。
 *
 * 生成规范：
 *   - 4空格缩进
 *   - 字符串双引号，` 作转义符
 *   - 热键体统一用 { } 块
 *   - LoopDoWhileNode → loop { ...body... if(cond) break }
 *   - 不支持的节点输出 "; [WARNING]..." 注释
 */
#pragma once
#include "AIR.h"
#include <string>
#include <vector>
#include <sstream>

class AhkGenerator : public AIR::IAIRVisitor {
public:
    explicit AhkGenerator(std::vector<AIR::AIRDiagnostic>& diags);
    std::string result() const { return m_out.str(); }

    void visit(AIR::ProgramNode&)        override;
    void visit(AIR::SequenceNode&)       override;
    void visit(AIR::FunctionDefNode&)    override;
    void visit(AIR::FunctionCallNode&)   override;
    void visit(AIR::TriggerNode&)        override;
    void visit(AIR::HotkeyNode&)         override;
    void visit(AIR::IfNode&)             override;
    void visit(AIR::LoopCountNode&)      override;
    void visit(AIR::LoopWhileNode&)      override;
    void visit(AIR::LoopDoWhileNode&)    override;
    void visit(AIR::LoopInfiniteNode&)   override;
    void visit(AIR::BreakNode&)          override;
    void visit(AIR::ContinueNode&)       override;
    void visit(AIR::ReturnNode&)         override;
    void visit(AIR::LabelNode&)          override;
    void visit(AIR::GotoNode&)           override;
    void visit(AIR::VarAssignNode&)      override;
    void visit(AIR::VarDeclareNode&)     override;
    void visit(AIR::KeyDownNode&)        override;
    void visit(AIR::KeyUpNode&)          override;
    void visit(AIR::KeyTapNode&)         override;
    void visit(AIR::KeySendStringNode&)  override;
    void visit(AIR::MouseMoveNode&)      override;
    void visit(AIR::MouseClickNode&)     override;
    void visit(AIR::MouseDownNode&)      override;
    void visit(AIR::MouseUpNode&)        override;
    void visit(AIR::MouseScrollNode&)    override;
    void visit(AIR::SleepNode&)          override;
    void visit(AIR::QueryKeyStateNode&)  override;
    void visit(AIR::ExprNumberNode&)     override;
    void visit(AIR::ExprStringNode&)     override;
    void visit(AIR::ExprBoolNode&)       override;
    void visit(AIR::ExprVarNode&)        override;
    void visit(AIR::ExprCallNode&)       override;
    void visit(AIR::ExprBinopNode&)      override;
    void visit(AIR::ExprUnopNode&)       override;
    void visit(AIR::ExprTernaryNode&)    override;
    void visit(AIR::ExprRandomRangeNode&)override;
    void visit(AIR::CommentNode&)        override;
    void visit(AIR::RawNode&)            override;

private:
    void        line(const std::string& s);
    void        write(const std::string& s);
    void        nl();
    std::string ind() const;
    void        push() { ++m_depth; }
    void        pop() { if (m_depth > 0) --m_depth; }
    std::string expr(AIR::AIRNode& node);   // 渲染表达式为内联字符串
    std::string keyStr(AIR::AIRKey k, const std::string& raw = {});
    std::string modPfx(AIR::ModMask m);
    std::string binOpStr(AIR::BinOp op);
    void        warn(const std::string& msg, int srcLine = 0);

    std::ostringstream               m_out;
    int                              m_depth = 0;
    std::vector<AIR::AIRDiagnostic>& m_diags;
};
/*
 * LogitechGenerator.h  —  AIR 树 → 罗技 G HUB Lua 宏生成器
 *
 * 生成的 Lua 脚本格式（标准罗技 G HUB / LGS 格式）：
 *
 *   function OnEvent(event, arg)
 *     if event == "MOUSE_BUTTON_PRESSED" and arg == 1 then
 *       repeat
 *         MoveMouseRelative(0, 3)
 *         Sleep(50)
 *       until not IsMouseButtonPressed(1)
 *     end
 *   end
 *
 * AIR 节点 → Lua 映射：
 *   SleepNode           → Sleep(ms)
 *   MouseMoveNode(REL)  → MoveMouseRelative(dx, dy)
 *   MouseMoveNode(ABS)  → MoveMouseTo(x, y)
 *   KeyDownNode         → PressKey("key")
 *   KeyUpNode           → ReleaseKey("key")
 *   KeyTapNode          → PressAndReleaseKey("key")
 *   MouseDownNode       → PressMouseButton(n)
 *   MouseUpNode         → ReleaseMouseButton(n)
 *   MouseClickNode      → PressMouseButton(n) + Sleep(0) + ReleaseMouseButton(n)
 *   MouseScrollNode     → MoveMouseWheel(±n)
 *   QueryKeyStateNode   → IsMouseButtonPressed(n)
 *   TriggerNode         → if event == "..." and arg == n then ... end
 *   HotkeyNode          → if event == "G_PRESSED" and arg == n then ... end
 *   LoopWhileNode       → while cond do ... end
 *   LoopDoWhileNode     → repeat ... until cond
 *   LoopCountNode       → for i=1,n do ... end
 *   LoopInfiniteNode    → while true do ... end
 *   IfNode              → if ... then ... [else ...] end
 *   ExprRandomRangeNode → math.random(min, max)
 */
#pragma once
#include "AIR.h"
#include <string>
#include <vector>
#include <sstream>

class LogitechGenerator : public AIR::IAIRVisitor {
public:
    explicit LogitechGenerator(std::vector<AIR::AIRDiagnostic>& diags);
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
    void        pop()  { if(m_depth>0) --m_depth; }
    std::string expr(AIR::AIRNode& node);
    std::string luaKey(AIR::AIRKey k, const std::string& raw={});
    int         mouseBtn(AIR::AIRKey k);
    std::string binOpStr(AIR::BinOp op);
    void        warn(const std::string& msg, int srcLine=0);

    std::ostringstream               m_out;
    int                              m_depth = 0;
    std::vector<AIR::AIRDiagnostic>& m_diags;
};

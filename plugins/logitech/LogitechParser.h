/*
 * LogitechParser.h  —  罗技 Lua 宏 → AIR 树
 *
 * 解析策略：
 *   手写递归下降，针对罗技宏实际用到的 Lua 子集。
 *   完整 Lua 语法非常复杂，本解析器只处理常见游戏宏模式：
 *
 *   支持：
 *     function OnEvent(event, arg) ... end  （触发器主体）
 *     function Name(...) ... end            （用户自定义函数）
 *     local/全局变量声明与赋值
 *     if ... then ... [elseif ... then ...] [else ...] end
 *     while ... do ... end
 *     repeat ... until ...
 *     for i = start, stop [, step] do ... end   → LoopCountNode
 *     break
 *     return [expr]
 *     Sleep(ms)
 *     PressKey("key") / ReleaseKey("key") / PressAndReleaseKey("key")
 *     PressMouseButton(n) / ReleaseMouseButton(n)
 *     MoveMouseRelative(dx, dy) / MoveMouseTo(x, y)
 *     MoveMouseWheel(clicks)
 *     IsMouseButtonPressed(n)  → QueryKeyStateNode（用于条件）
 *     -- 单行注释
 *     [[...]] 多行字符串（忽略）
 *
 *   容错：无法解析的行包装为 RawNode + WARNING。
 *
 * AIR 结构（标准罗技压枪宏）：
 *   ProgramNode
 *   └── TriggerNode(MouseButtonDown, LEFT)
 *       └── SequenceNode
 *           └── LoopDoWhileNode
 *               ├── [0] SequenceNode（循环体）
 *               │   ├── MouseMoveNode(CoordRelative)
 *               │   └── SleepNode
 *               └── [1] ExprUnopNode(NOT)
 *                   └── QueryKeyStateNode(LEFT)
 */
#pragma once
#include "AIR.h"
#include <string>
#include <vector>

class LogitechParser {
public:
    LogitechParser(const std::string& source, std::vector<AIR::AIRDiagnostic>& diags);
    AIR::AIRNodePtr parse();

private:
    // ── Token ────────────────────────────────────────────────────────────────
    enum class TK {
        NUMBER, STRING, IDENT,
        PLUS, MINUS, STAR, SLASH, PERCENT, CARET,  // 算术（^ = 幂）
        EQ2,    // ==
        NEQ,    // ~=（Lua 的不等于）
        LT, GT, LTE, GTE,
        AND2, OR2,  // and or（关键字，tokenize 后变 IDENT，特判）
        NOT2,       // not（关键字）
        DOTDOT,     // .. 字符串拼接
        ASSIGN,     // =
        COMMA, LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE,
        DOT, COLON, SEMICOLON, HASH,
        NEWLINE, END, UNKNOWN
    };
    struct Token { TK type; std::string value; int line; };

    void         tokenize();
    const Token& peek(int offset = 0) const;
    Token        consume();
    bool         check(TK t) const;
    bool         checkKw(const char* kw) const; // 检查 IDENT 是否匹配关键字（不敏感）
    void         skipNL();
    void         skipToEnd(); // 跳到行尾或 end 关键字
    void         addWarn(int ln, const std::string& msg);
    void         addError(const std::string& msg);

    // ── 解析 ─────────────────────────────────────────────────────────────────
    AIR::AIRNodePtr parseProgram();
    AIR::AIRNodePtr parseTopLevel();
    AIR::AIRNodePtr parseFuncDef();     // function Name(...) ... end
    AIR::AIRNodePtr parseBlock();       // 语句序列，直到 end/else/elseif/until/EOF
    AIR::AIRNodePtr parseStatement();
    AIR::AIRNodePtr parseLocal();       // local name [= expr]
    AIR::AIRNodePtr parseAssignOrCall();// name = expr  或  name(args)
    AIR::AIRNodePtr parseIf();
    AIR::AIRNodePtr parseWhile();
    AIR::AIRNodePtr parseRepeat();      // repeat...until
    AIR::AIRNodePtr parseFor();         // for i=start,stop do...end
    AIR::AIRNodePtr parseReturn();
    AIR::AIRNodePtr parseComment();

    // 内置 API 调用
    AIR::AIRNodePtr parseApiCall(const std::string& name, int line);

    // 表达式（简化，足够覆盖宏常见写法）
    AIR::AIRNodePtr parseExpr();
    AIR::AIRNodePtr parseOrExpr();
    AIR::AIRNodePtr parseAndExpr();
    AIR::AIRNodePtr parseNotExpr();
    AIR::AIRNodePtr parseCompare();
    AIR::AIRNodePtr parseConcat();
    AIR::AIRNodePtr parseAddSub();
    AIR::AIRNodePtr parseMulDiv();
    AIR::AIRNodePtr parseUnary();
    AIR::AIRNodePtr parsePower();
    AIR::AIRNodePtr parsePrimary();

    // 辅助：解析函数调用参数列表，返回参数 vector（调用方已消耗 '('）
    std::vector<AIR::AIRNodePtr> parseArgList();

    // 判断当前块是否结束（end/else/elseif/until/EOF）
    bool isBlockEnd() const;

    std::string                      m_source;
    std::vector<Token>               m_tokens;
    size_t                           m_pos = 0;
    std::vector<AIR::AIRDiagnostic>& m_diags;
    static const Token               kEOF;
};

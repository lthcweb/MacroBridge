/*
 * AhkParser.h  —  AHK v2 脚本文本 → AIR 树
 *
 * 手写递归下降解析器，支持：
 *   热键 [~][修饰符]键::  / 函数定义 / if while loop / 变量赋值
 *   Send / SendText / MouseMove / Click / MouseClick / Sleep
 *   GetKeyState() / Random() / 表达式（含运算符优先级）
 *   ; 注释 / local global static 声明
 *
 * 容错：无法解析的行包装为 RawNode + WARNING，不中断整体解析。
 */
#pragma once
#include "AIR.h"
#include <string>
#include <vector>

class AhkParser {
public:
    AhkParser(const std::string& source, std::vector<AIR::AIRDiagnostic>& diags);
    AIR::AIRNodePtr parse();

private:
    enum class TK {
        NUMBER, STRING, IDENT,
        ASSIGN, PLUS, MINUS, STAR, SLASH, PERCENT, STARSTAR,
        EQ, NEQ, LT, GT, LTE, GTE, AND, OR, NOT, DOT,
        QUESTION, COLON, DCOLON, COMMA, LPAREN, RPAREN,
        LBRACE, RBRACE, TILDE, HASH, NEWLINE, END, UNKNOWN
    };
    struct Token { TK type; std::string value; int line; };

    void         tokenize();
    const Token& peek(int offset = 0) const;
    Token        consume();
    bool         check(TK t) const;
    bool         checkId(const char* name) const;
    void         skipNL();
    void         skipToEOL();
    void         addWarn(int ln, const std::string& msg);

    AIR::AIRNodePtr parseProgram();
    AIR::AIRNodePtr parseTopLevel();
    AIR::AIRNodePtr parseHotkey();
    AIR::AIRNodePtr parseFuncDef();
    AIR::AIRNodePtr parseBlock();
    AIR::AIRNodePtr parseStatement();
    AIR::AIRNodePtr parseIf();
    AIR::AIRNodePtr parseWhile();
    AIR::AIRNodePtr parseLoop();
    AIR::AIRNodePtr parseReturn();
    AIR::AIRNodePtr parseVarDecl();
    AIR::AIRNodePtr parseAssignOrCall();
    AIR::AIRNodePtr parseSend();
    AIR::AIRNodePtr parseMouseMove();
    AIR::AIRNodePtr parseClick();
    AIR::AIRNodePtr parseSleep();
    AIR::AIRNodePtr parseComment();
    AIR::AIRNodePtr parseSendKey(const std::string& spec);
    uint8_t         parseModifiers();

    // 表达式（优先级从低到高）
    AIR::AIRNodePtr parseExpr();
    AIR::AIRNodePtr parseTernary();
    AIR::AIRNodePtr parseOr();
    AIR::AIRNodePtr parseAnd();
    AIR::AIRNodePtr parseEquality();
    AIR::AIRNodePtr parseRelational();
    AIR::AIRNodePtr parseConcat();
    AIR::AIRNodePtr parseAddSub();
    AIR::AIRNodePtr parseMulDiv();
    AIR::AIRNodePtr parsePower();
    AIR::AIRNodePtr parseUnary();
    AIR::AIRNodePtr parsePrimary();

    std::string m_source;
    std::vector<Token> m_tokens;
    size_t m_pos = 0;
    std::vector<AIR::AIRDiagnostic>& m_diags;
    static const Token kEOF;
};
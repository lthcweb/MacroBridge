/*
 * AhkParser.cpp  —  AHK v2 脚本 → AIR 树
 */
#include "AhkParser.h"
#include "AhkKeyMap.h"
#include <cctype>
#include <algorithm>
#include <sstream>
using namespace AIR;

const AhkParser::Token AhkParser::kEOF = { TK::END,"",0 };

AhkParser::AhkParser(const std::string& src, std::vector<AIRDiagnostic>& d)
    : m_source(src), m_diags(d) {
    tokenize();
}

// ============================================================================
//  词法分析
// ============================================================================
void AhkParser::tokenize() {
    const std::string& s = m_source;
    size_t i = 0; int line = 1;
    auto push = [&](TK t, std::string v) { m_tokens.push_back({ t,std::move(v),line }); };
    while (i < s.size()) {
        char c = s[i];
        if (c == '\n' || c == '\r') {
            if (c == '\r' && i + 1 < s.size() && s[i + 1] == '\n') {
                ++i; // 合并处理 \r\n
            }
            push(TK::NEWLINE, "\n");
            ++line;
            ++i;
            continue;
        }
        if (c == ' ' || c == '\t') { ++i;continue; }
        if (c == ';') { // 注释
            size_t st = i + 1;
            // 同时也查找 \r 或 \n
            while (i < s.size() && s[i] != '\n' && s[i] != '\r') ++i;

            std::string t = s.substr(st, i - st);
            size_t b = t.find_first_not_of(" \t");
            t = (b == std::string::npos) ? "" : t.substr(b);

            // 去掉注释末尾可能残余的 \r (针对 substr 拿到的内容)
            if (!t.empty() && t.back() == '\r') t.pop_back();

            push(TK::IDENT, "__CMT__:" + t);
            // 注意：这里不要 ++i，让外层循环去处理接下来的 \r 或 \n
            continue;
        }
        if (c == '"') {// 字符串
            ++i; std::string str;
            while (i < s.size() && s[i] != '"') {
                if (s[i] == '`' && i + 1 < s.size()) {
                    char e = s[i + 1];
                    switch (e) { case 'n':str += '\n';break;case 't':str += '\t';break;case '"':str += '"';break;case '`':str += '`';break;default:str += '`';str += e; }
                                         i += 2;
                }
                else str += s[i++];
            }
            if (i < s.size())++i; push(TK::STRING, str); continue;
        }
        if (std::isdigit((unsigned char)c)) {
            size_t st = i; while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i] == '.'))++i;
            push(TK::NUMBER, s.substr(st, i - st)); continue;
        }
        if (std::isalpha((unsigned char)c) || c == '_') {
            size_t st = i; while (i < s.size() && (std::isalnum((unsigned char)s[i]) || s[i] == '_'))++i;
            push(TK::IDENT, s.substr(st, i - st)); continue;
        }
        if (i + 1 < s.size()) {
            std::string two = s.substr(i, 2);
            if (two == ":=") { push(TK::ASSIGN, ":=");i += 2;continue; }
            if (two == "::") { push(TK::DCOLON, "::");i += 2;continue; }
            if (two == "==") { push(TK::EQ, "==");i += 2;continue; }
            if (two == "!=") { push(TK::NEQ, "!=");i += 2;continue; }
            if (two == "<=") { push(TK::LTE, "<=");i += 2;continue; }
            if (two == ">=") { push(TK::GTE, ">=");i += 2;continue; }
            if (two == "&&") { push(TK::AND, "&&");i += 2;continue; }
            if (two == "||") { push(TK::OR, "||");i += 2;continue; }
            if (two == "**") { push(TK::STARSTAR, "**");i += 2;continue; }
        }
        switch (c) {
        case '+':push(TK::PLUS, "+");break; case '-':push(TK::MINUS, "-");break;
        case '*':push(TK::STAR, "*");break; case '/':push(TK::SLASH, "/");break;
        case '%':push(TK::PERCENT, "%");break; case '<':push(TK::LT, "<");break;
        case '>':push(TK::GT, ">");break; case '!':push(TK::NOT, "!");break;
        case '.':push(TK::DOT, ".");break; case '?':push(TK::QUESTION, "?");break;
        case ':':push(TK::COLON, ":");break; case ',':push(TK::COMMA, ",");break;
        case '(':push(TK::LPAREN, "(");break; case ')':push(TK::RPAREN, ")");break;
        case '{':push(TK::LBRACE, "{");break; case '}':push(TK::RBRACE, "}");break;
        case '~':push(TK::TILDE, "~");break; case '#':push(TK::HASH, "#");break;
        default: push(TK::UNKNOWN, std::string(1, c)); break;
        }
        ++i;
    }
    push(TK::END, "");
}

const AhkParser::Token& AhkParser::peek(int offset) const {
    size_t idx = m_pos + (size_t)offset;
    return idx < m_tokens.size() ? m_tokens[idx] : kEOF;
}
AhkParser::Token AhkParser::consume() { return m_pos < m_tokens.size() ? m_tokens[m_pos++] : kEOF; }
bool  AhkParser::check(TK t) const { return peek().type == t; }
bool  AhkParser::checkId(const char* name) const {
    if (!check(TK::IDENT)) return false;
    std::string lo = peek().value, ref(name);
    std::transform(lo.begin(), lo.end(), lo.begin(), [](unsigned char c) {return (char)std::tolower(c);});
    std::transform(ref.begin(), ref.end(), ref.begin(), [](unsigned char c) {return (char)std::tolower(c);});
    return lo == ref;
}
void AhkParser::skipNL() { while (check(TK::NEWLINE))consume(); }
void AhkParser::skipToEOL() { while (!check(TK::NEWLINE) && !check(TK::END))consume(); }
void AhkParser::addWarn(int ln, const std::string& msg) { m_diags.push_back({ DiagLevel::Warning,ln,msg,{} }); }

// ============================================================================
//  解析
// ============================================================================
AIRNodePtr AhkParser::parse() { return parseProgram(); }

AIRNodePtr AhkParser::parseProgram() {
    auto prog = ProgramNode::make(); skipNL();
    while (!check(TK::END)) { auto n = parseTopLevel(); if (n) prog->addChild(std::move(n)); skipNL(); }
    return prog;
}

AIRNodePtr AhkParser::parseTopLevel() {
    if (check(TK::IDENT) && peek().value.substr(0, 7) == "__CMT__") return parseComment();
    if (check(TK::HASH)) {
        int l = peek().line;std::string r;while (!check(TK::NEWLINE) && !check(TK::END))r += consume().value + " ";
        addWarn(l, "#指令暂不支持: " + r); return RawNode::make(r);
    }
    // 热键检测：向前看是否有 ::
    {
        size_t la = m_pos; if (m_tokens[la].type == TK::TILDE)++la;
        while (la < m_tokens.size() && m_tokens[la].type != TK::DCOLON && m_tokens[la].type != TK::NEWLINE && m_tokens[la].type != TK::END)++la;
        if (la < m_tokens.size() && m_tokens[la].type == TK::DCOLON) return parseHotkey();
    }
    // 函数定义：IDENT (
    if (check(TK::IDENT) && m_pos + 1 < m_tokens.size() && m_tokens[m_pos + 1].type == TK::LPAREN) return parseFuncDef();
    return parseStatement();
}

uint8_t AhkParser::parseModifiers() {
    uint8_t m = 0;
    while (true) {
        if (check(TK::NOT)) { m |= (uint8_t)AIR::ModMask::Alt;consume();continue; }
        if (check(TK::PLUS)) { m |= (uint8_t)AIR::ModMask::Shift;consume();continue; }
        if (check(TK::HASH)) { m |= (uint8_t)AIR::ModMask::Win;consume();continue; }
        if (check(TK::UNKNOWN) && peek().value == "^") { m |= (uint8_t)AIR::ModMask::Ctrl;consume();continue; }
        break;
    }
    return m;
}

AIRNodePtr AhkParser::parseHotkey() {
    bool pass = false; if (check(TK::TILDE)) { pass = true;consume(); }
    uint8_t mods = parseModifiers();
    std::string keyName;
    if (check(TK::IDENT)) keyName = consume().value;
    else if (!check(TK::DCOLON)) keyName = consume().value;
    if (check(TK::DCOLON)) consume(); // ::
    AIRKey key = AhkKeyMap::AhkToAIR(keyName);
    auto hk = HotkeyNode::make(key, static_cast<ModMask>(mods), pass);
    skipNL();
    AIRNodePtr body;
    if (check(TK::LBRACE)) body = parseBlock();
    else { auto sq = SequenceNode::make(); auto st = parseStatement(); if (st) sq->addChild(std::move(st)); body = std::move(sq); }
    hk->addChild(std::move(body)); return hk;
}

AIRNodePtr AhkParser::parseFuncDef() {
    std::string name = consume().value; consume(); // (
    std::vector<std::string> params;
    while (!check(TK::RPAREN) && !check(TK::END)) { if (check(TK::IDENT))params.push_back(consume().value);if (check(TK::COMMA))consume(); }
    if (check(TK::RPAREN))consume(); skipNL();
    auto fd = FunctionDefNode::make(name, params); fd->addChild(parseBlock()); return fd;
}

AIRNodePtr AhkParser::parseBlock() {
    if (check(TK::LBRACE))consume(); auto sq = SequenceNode::make(); skipNL();
    while (!check(TK::RBRACE) && !check(TK::END)) { auto st = parseStatement();if (st)sq->addChild(std::move(st));skipNL(); }
    if (check(TK::RBRACE))consume(); return sq;
}

AIRNodePtr AhkParser::parseStatement() {
    while (check(TK::NEWLINE))consume();
    if (check(TK::END) || check(TK::RBRACE)) return nullptr;
    int ln = peek().line;
    if (check(TK::IDENT) && peek().value.substr(0, 7) == "__CMT__") return parseComment();
    if (check(TK::IDENT)) {
        std::string kw = peek().value, lo = kw;
        std::transform(lo.begin(), lo.end(), lo.begin(), [](unsigned char c) {return (char)std::tolower(c);});
        if (lo == "if") return parseIf();
        if (lo == "while") return parseWhile();
        if (lo == "loop") return parseLoop();
        if (lo == "break") { consume();skipToEOL();return BreakNode::make(); }
        if (lo == "continue") { consume();skipToEOL();return ContinueNode::make(); }
        if (lo == "return") return parseReturn();
        if (lo == "local" || lo == "global" || lo == "static") return parseVarDecl();
        if (lo == "send" || lo == "sendtext") return parseSend();
        if (lo == "mousemove") return parseMouseMove();
        if (lo == "click" || lo == "mouseclick") return parseClick();
        if (lo == "sleep") return parseSleep();
        if (m_pos + 1 < m_tokens.size() && (m_tokens[m_pos + 1].type == TK::LPAREN || m_tokens[m_pos + 1].type == TK::ASSIGN))
            return parseAssignOrCall();
    }
    std::string raw; while (!check(TK::NEWLINE) && !check(TK::END) && !check(TK::RBRACE)) raw += consume().value + " ";
    if (!raw.empty()) addWarn(ln, "无法解析: " + raw);
    return raw.empty() ? nullptr : RawNode::make(raw);
}

AIRNodePtr AhkParser::parseIf() {
    consume(); bool hp = check(TK::LPAREN); if (hp)consume();
    auto n = IfNode::make(); n->addChild(parseExpr());
    if (hp && check(TK::RPAREN))consume(); skipNL(); n->addChild(parseBlock()); skipNL();
    if (checkId("else")) { consume();skipNL();n->addChild(checkId("if") ? parseIf() : parseBlock()); }
    return n;
}
AIRNodePtr AhkParser::parseWhile() {
    consume(); bool hp = check(TK::LPAREN); if (hp)consume();
    auto n = LoopWhileNode::make(); n->addChild(parseExpr());
    if (hp && check(TK::RPAREN))consume(); skipNL(); n->addChild(parseBlock()); return n;
}
AIRNodePtr AhkParser::parseLoop() {
    consume();
    if (check(TK::LBRACE) || check(TK::NEWLINE)) { skipNL(); auto n = LoopInfiniteNode::make(); n->addChild(parseBlock()); return n; }
    auto n = LoopCountNode::make(); n->addChild(parseExpr()); skipNL(); n->addChild(parseBlock()); return n;
}
AIRNodePtr AhkParser::parseReturn() {
    consume(); auto n = ReturnNode::make();
    if (!check(TK::NEWLINE) && !check(TK::END) && !check(TK::RBRACE)) n->addChild(parseExpr());
    return n;
}
AIRNodePtr AhkParser::parseVarDecl() {
    std::string kw = consume().value; std::string lo = kw;
    std::transform(lo.begin(), lo.end(), lo.begin(), [](unsigned char c) {return (char)std::tolower(c);});
    VarScope sc = VarScope::Local; if (lo == "global")sc = VarScope::Global; else if (lo == "static")sc = VarScope::Static;
    std::string nm; if (check(TK::IDENT)) nm = consume().value;
    auto n = VarDeclareNode::make(nm, sc);
    if (check(TK::ASSIGN)) { consume();n->addChild(parseExpr()); } return n;
}
AIRNodePtr AhkParser::parseAssignOrCall() {
    std::string nm = consume().value;
    if (check(TK::ASSIGN)) { consume(); auto n = VarAssignNode::make(nm); n->addChild(parseExpr()); return n; }
    if (check(TK::LPAREN)) {
        consume(); auto n = FunctionCallNode::make(nm);
        while (!check(TK::RPAREN) && !check(TK::END)) { n->addChild(parseExpr());if (check(TK::COMMA))consume(); }
        if (check(TK::RPAREN))consume(); return n;
    }
    addWarn(peek().line, "无法解析标识符后: " + nm); 
    return RawNode::make(nm);
}

AIRNodePtr AhkParser::parseSend() {
    int ln = peek().line;
    std::string cmd = peek().value; std::string lo = cmd;
    std::transform(lo.begin(), lo.end(), lo.begin(), [](unsigned char c) {return (char)std::tolower(c);});
    consume();
    if (lo == "sendtext") { std::string t; if (check(TK::STRING))t = consume().value; return KeySendStringNode::make(t); }
    if (!check(TK::STRING)) { addWarn(ln, "Send 后需字符串");return RawNode::make("Send"); }
    std::string str = consume().value;
    auto sq = SequenceNode::make(); size_t i = 0;
    while (i < str.size()) {
        if (str[i] == '{') {
            size_t e = str.find('}', i);if (e == std::string::npos)break;
            auto kn = parseSendKey(str.substr(i + 1, e - i - 1));if (kn)sq->addChild(std::move(kn));i = e + 1;
        }
        else {
            std::string ch(1, str[i]); AIRKey k = AhkKeyMap::AhkToAIR(ch);
            auto t = KeyTapNode::make(k,1,0,ch);  sq->addChild(std::move(t));++i;
        }
    }
    if (sq->children.size() == 1) return std::move(sq->children[0]);
    return sq;
}
AIRNodePtr AhkParser::parseSendKey(const std::string& spec) {
    std::istringstream ss(spec); std::string kn, rest; ss >> kn; int cnt = 1; std::string mod;
    if (ss >> rest) {
        std::string rl = rest;std::transform(rl.begin(), rl.end(), rl.begin(), [](unsigned char c) {return (char)std::tolower(c);});
        if (rl == "down")mod = "down"; else if (rl == "up")mod = "up"; else try { cnt = std::stoi(rest); }
        catch (...) {}
    }
    AIRKey k = AhkKeyMap::AhkToAIR(kn);
    if (mod == "down") { auto n = KeyDownNode::make(k,kn); return n;   }
    if (mod == "up") { auto n = KeyUpNode::make(k,kn); return n;    }
    auto n = KeyTapNode::make(k, cnt,0,kn); 
    return n;
}
AIRNodePtr AhkParser::parseMouseMove() {
    consume(); CoordType ct = CoordType::CoordAbsolute;
    auto xe = parseExpr(); if (check(TK::COMMA))consume();
    auto ye = parseExpr();
    if (check(TK::COMMA)) { consume();parseExpr(); } // speed
    if (check(TK::COMMA)) {
        consume(); if (check(TK::STRING)) {
            std::string m = peek().value;
            std::transform(m.begin(), m.end(), m.begin(), [](unsigned char c) {return (char)std::tolower(c);});
            if (m == "r")ct = CoordType::CoordRelative; consume();
        }
    }
    auto n = MouseMoveNode::make(ct); n->addChild(std::move(xe)); n->addChild(std::move(ye)); return n;
}
AIRNodePtr AhkParser::parseClick() {
    int ln = peek().line; std::string cmd = peek().value; std::string lo = cmd;
    std::transform(lo.begin(), lo.end(), lo.begin(), [](unsigned char c) {return (char)std::tolower(c);});
    consume();
    if (lo == "mouseclick") {// MouseClick btn, x, y, count
        AIRKey btn = AIRKey::KEY_MOUSE_LEFT; int cnt = 1;
        if (check(TK::IDENT)) {
            std::string b = peek().value;std::string bl = b;
            std::transform(bl.begin(), bl.end(), bl.begin(), [](unsigned char c) {return (char)std::tolower(c);});
            if (bl == "right")btn = AIRKey::KEY_MOUSE_RIGHT; else if (bl == "middle")btn = AIRKey::KEY_MOUSE_MIDDLE;
            consume();
        }
        if (check(TK::COMMA))consume();
        AIRNodePtr xe, ye;
        if (!check(TK::NEWLINE) && !check(TK::END)) { xe = parseExpr();if (check(TK::COMMA)) { consume();ye = parseExpr(); } }
        if (check(TK::COMMA)) { consume();if (check(TK::NUMBER))try { cnt = std::stoi(consume().value); } catch (...) {} }
        while (!check(TK::NEWLINE) && !check(TK::END))consume();
        auto n = MouseClickNode::make(btn, cnt); if (xe)n->addChild(std::move(xe)); if (ye)n->addChild(std::move(ye)); return n;
    }
    // Click
    if (check(TK::NEWLINE) || check(TK::END)) return MouseClickNode::make(AIRKey::KEY_MOUSE_LEFT, 1);
    AIRKey btn = AIRKey::KEY_MOUSE_LEFT; bool dn = false, up = false; int cnt = 1;
    if (check(TK::STRING)) {
        std::string opt = consume().value; std::istringstream ss(opt); std::string tok;
        while (ss >> tok) {
            std::string tl = tok;std::transform(tl.begin(), tl.end(), tl.begin(), [](unsigned char c) {return (char)std::tolower(c);});
            if (tl == "right")btn = AIRKey::KEY_MOUSE_RIGHT; else if (tl == "middle")btn = AIRKey::KEY_MOUSE_MIDDLE;
            else if (tl == "down")dn = true; else if (tl == "up")up = true; else try { cnt = std::stoi(tok); }
            catch (...) {}
        }
    }
    else {
        while (!check(TK::NEWLINE) && !check(TK::END)) {
            if (check(TK::IDENT)) {
                std::string v = peek().value;std::string vl = v;
                std::transform(vl.begin(), vl.end(), vl.begin(), [](unsigned char c) {return (char)std::tolower(c);});
                if (vl == "right")btn = AIRKey::KEY_MOUSE_RIGHT; else if (vl == "middle")btn = AIRKey::KEY_MOUSE_MIDDLE;
                else if (vl == "down")dn = true; else if (vl == "up")up = true; consume();
            }
            else if (check(TK::NUMBER))try { cnt = std::stoi(consume().value); }
            catch (...) { consume(); }
            else consume();
        }
    }
    if (dn) return MouseDownNode::make(btn);
    if (up) return MouseUpNode::make(btn);
    return MouseClickNode::make(btn, cnt);
}
AIRNodePtr AhkParser::parseSleep() {
    consume(); auto n = SleepNode::make(0); n->children.clear(); n->addChild(parseExpr()); return n;
}
AIRNodePtr AhkParser::parseComment() {
    std::string raw = consume().value; std::string text = raw.substr(8); // "__CMT__:"
    return CommentNode::make(text, false);
}

// ============================================================================
//  表达式（递归下降，优先级从低到高）
// ============================================================================
AIRNodePtr AhkParser::parseExpr() { return parseTernary(); }
AIRNodePtr AhkParser::parseTernary() {
    auto l = parseOr(); if (check(TK::QUESTION)) { consume(); auto n = ExprTernaryNode::make(); n->addChild(std::move(l)); n->addChild(parseOr()); if (check(TK::COLON))consume(); n->addChild(parseOr()); return n; } return l;
}
AIRNodePtr AhkParser::parseOr() {
    auto l = parseAnd(); while (check(TK::OR)) { consume(); auto n = ExprBinopNode::make(BinOp::OpOr); n->addChild(std::move(l)); n->addChild(parseAnd()); l = std::move(n); } return l;
}
AIRNodePtr AhkParser::parseAnd() {
    auto l = parseEquality(); while (check(TK::AND)) { consume(); auto n = ExprBinopNode::make(BinOp::OpAnd); n->addChild(std::move(l)); n->addChild(parseEquality()); l = std::move(n); } return l;
}
AIRNodePtr AhkParser::parseEquality() {
    auto l = parseRelational();
    while (check(TK::EQ) || check(TK::NEQ)) { BinOp op = check(TK::EQ) ? BinOp::Eq : BinOp::Neq; consume(); auto n = ExprBinopNode::make(op); n->addChild(std::move(l)); n->addChild(parseRelational()); l = std::move(n); }
    return l;
}
AIRNodePtr AhkParser::parseRelational() {
    auto l = parseConcat();
    while (check(TK::LT) || check(TK::GT) || check(TK::LTE) || check(TK::GTE)) {
        BinOp op = check(TK::LT) ? BinOp::Lt : check(TK::GT) ? BinOp::Gt : check(TK::LTE) ? BinOp::Lte : BinOp::Gte;
        consume(); auto n = ExprBinopNode::make(op); n->addChild(std::move(l)); n->addChild(parseConcat()); l = std::move(n);
    }
    return l;
}
AIRNodePtr AhkParser::parseConcat() {
    auto l = parseAddSub(); while (check(TK::DOT)) { consume(); auto n = ExprBinopNode::make(BinOp::Concat); n->addChild(std::move(l)); n->addChild(parseAddSub()); l = std::move(n); } return l;
}
AIRNodePtr AhkParser::parseAddSub() {
    auto l = parseMulDiv();
    while (check(TK::PLUS) || check(TK::MINUS)) { BinOp op = check(TK::PLUS) ? BinOp::Add : BinOp::Sub; consume(); auto n = ExprBinopNode::make(op); n->addChild(std::move(l)); n->addChild(parseMulDiv()); l = std::move(n); }
    return l;
}
AIRNodePtr AhkParser::parseMulDiv() {
    auto l = parsePower();
    while (check(TK::STAR) || check(TK::SLASH) || check(TK::PERCENT)) {
        BinOp op = check(TK::STAR) ? BinOp::Mul : check(TK::SLASH) ? BinOp::Div : BinOp::OpMod;
        consume(); auto n = ExprBinopNode::make(op); n->addChild(std::move(l)); n->addChild(parsePower()); l = std::move(n);
    }
    return l;
}
AIRNodePtr AhkParser::parsePower() {
    auto l = parseUnary(); if (check(TK::STARSTAR)) { consume(); auto n = ExprBinopNode::make(BinOp::Pow); n->addChild(std::move(l)); n->addChild(parseUnary()); return n; } return l;
}
AIRNodePtr AhkParser::parseUnary() {
    if (check(TK::NOT)) { consume(); auto n = ExprUnopNode::make(UnOp::OpNot); n->addChild(parseUnary()); return n; }
    if (check(TK::MINUS)) { consume(); auto n = ExprUnopNode::make(UnOp::Neg); n->addChild(parseUnary()); return n; }
    return parsePrimary();
}
AIRNodePtr AhkParser::parsePrimary() {
    int ln = peek().line;
    if (check(TK::NUMBER)) { double v = std::stod(peek().value);consume();return ExprNumberNode::make(v); }
    if (check(TK::STRING)) { std::string s = consume().value;return ExprStringNode::make(s); }
    if (check(TK::LPAREN)) { consume();auto e = parseExpr();if (check(TK::RPAREN))consume();return e; }
    if (check(TK::IDENT)) {
        std::string nm = peek().value, lo = nm;
        std::transform(lo.begin(), lo.end(), lo.begin(), [](unsigned char c) {return (char)std::tolower(c);});
        if (lo == "true") { consume();return ExprBoolNode::make(true); }
        if (lo == "false") { consume();return ExprBoolNode::make(false); }
        consume();
        if (check(TK::LPAREN)) {
            consume();
            if (lo == "random") {// Random(min,max)
                AIRNodePtr mn, mx; if (!check(TK::RPAREN)) { mn = parseExpr();if (check(TK::COMMA)) { consume();mx = parseExpr(); } }
                if (check(TK::RPAREN))consume();
                if (mn && mx) { auto n = std::make_unique<ExprRandomRangeNode>();n->addChild(std::move(mn));n->addChild(std::move(mx));return n; }
                auto n = ExprCallNode::make("Random"); if (mn)n->addChild(std::move(mn)); return n;
            }
            if (lo == "getkeystate") {// GetKeyState("Key","P")
                std::string ks; if (check(TK::STRING))ks = consume().value;
                while (!check(TK::RPAREN) && !check(TK::END))consume();
                if (check(TK::RPAREN))consume();
                return QueryKeyStateNode::make(AhkKeyMap::AhkToAIR(ks));
            }
            auto n = ExprCallNode::make(nm);
            while (!check(TK::RPAREN) && !check(TK::END)) { n->addChild(parseExpr());if (check(TK::COMMA))consume(); }
            if (check(TK::RPAREN))consume(); return n;
        }
        return ExprVarNode::make(nm);
    }
    addWarn(ln, "无法解析token: " + peek().value);
    return ExprNumberNode::make(0);
}
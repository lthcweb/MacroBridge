/*
 * LogitechParser.cpp  —  罗技 Lua 宏 → AIR 树实现
 */
#include "LogitechParser.h"
#include "LogitechKeyMap.h"
#include <cctype>
#include <algorithm>
#include <sstream>
#include <stdexcept>
using namespace AIR;

const LogitechParser::Token LogitechParser::kEOF = {TK::END,"",0};

// ============================================================================
//  构造
// ============================================================================
LogitechParser::LogitechParser(const std::string& src, std::vector<AIRDiagnostic>& d)
    : m_source(src), m_diags(d) { tokenize(); }

// ============================================================================
//  词法分析
// ============================================================================
void LogitechParser::tokenize()
{
    const std::string& s = m_source;
    size_t i = 0; int line = 1;
    auto push = [&](TK t, std::string v){ m_tokens.push_back({t, std::move(v), line}); };

    while (i < s.size()) {
        char c = s[i];

        if (c == '\n') { push(TK::NEWLINE,"\n"); ++line; ++i; continue; }
        if (c == '\r') { ++i; continue; }
        if (c == ' ' || c == '\t') { ++i; continue; }

        // -- 单行注释
        if (c == '-' && i + 1 < s.size() && s[i + 1] == '-') {
            // 检查是否为 --[[ 多行注释
            if (i + 3 < s.size() && s[i + 2] == '[' && s[i + 3] == '[') {
                i += 4;
                while (i + 1 < s.size() && !(s[i] == ']' && s[i + 1] == ']')) {
                    // 兼容多种换行符增加行号
                    if (s[i] == '\n') {
                        ++line;
                    }
                    else if (s[i] == '\r' && (i + 1 >= s.size() || s[i + 1] != '\n')) {
                        // 只有单独的 \r 时才加行号（标准 \r\n 会在 \n 处加）
                        ++line;
                    }
                    ++i;
                }
                if (i + 1 < s.size()) i += 2; // 跳过 ]]
                continue;
            }

            // 处理单行注释
            size_t st = i + 2;
            // 修改点：同时查找 \n 和 \r
            while (i < s.size() && s[i] != '\n' && s[i] != '\r') {
                ++i;
            }

            std::string t = s.substr(st, i - st);
            // 清理首尾空白
            size_t first = t.find_first_not_of(" \t");
            if (first == std::string::npos) {
                t = "";
            }
            else {
                size_t last = t.find_last_not_of(" \t\r"); // 额外清理末尾可能存在的 \r
                t = t.substr(first, (last - first + 1));
            }

            push(TK::IDENT, "__CMT__:" + t);
            // 注意：此处不自增 i，让外层循环的换行逻辑去处理 \n 或 \r
            continue;
        }

        // [[ 多行字符串
        if (c == '[' && i + 1 < s.size() && s[i + 1] == '[') {
            i += 2; std::string str;
            while (i + 1 < s.size() && !(s[i] == ']' && s[i + 1] == ']')) {
                // 记录行号：兼容 \r\n, \n, \r
                if (s[i] == '\n') ++line;
                else if (s[i] == '\r' && s[i + 1] != '\n') ++line;

                str += s[i++];
            }
            if (i + 1 < s.size()) i += 2;
            push(TK::STRING, str);
            continue;
        }

        // 字符串 "..." 或 '...'
        if (c == '"' || c == '\'') {
            char q = c; ++i; std::string str;
            while (i < s.size() && s[i] != q) {
                if (s[i]=='\\' && i+1 < s.size()) {
                    char e = s[i+1];
                    switch(e){ case 'n':str+='\n';break; case 't':str+='\t';break;
                               case '"':str+='"'; break; case '\'':str+='\'';break;
                               case '\\':str+='\\';break; default:str+='\\';str+=e; }
                    i += 2;
                } else { str += s[i++]; }
            }
            if (i < s.size()) ++i;
            push(TK::STRING, str); continue;
        }

        // 数字（含负数前缀由调用方处理）
        if (std::isdigit((unsigned char)c) ||
            (c=='.' && i+1<s.size() && std::isdigit((unsigned char)s[i+1]))) {
            size_t st = i;
            while (i<s.size() && (std::isdigit((unsigned char)s[i]) || s[i]=='.')) ++i;
            push(TK::NUMBER, s.substr(st, i-st)); continue;
        }

        // 标识符 / 关键字
        if (std::isalpha((unsigned char)c) || c=='_') {
            size_t st = i;
            while (i<s.size() && (std::isalnum((unsigned char)s[i]) || s[i]=='_')) ++i;
            push(TK::IDENT, s.substr(st, i-st)); continue;
        }

        // 双字符运算符
        if (i+1 < s.size()) {
            std::string two = s.substr(i,2);
            if (two=="=="){push(TK::EQ2,"==");i+=2;continue;}
            if (two=="~="){push(TK::NEQ,"~=");i+=2;continue;}
            if (two=="<="){push(TK::LTE,"<=");i+=2;continue;}
            if (two==">="){push(TK::GTE,">=");i+=2;continue;}
            if (two==".."){push(TK::DOTDOT,"..");i+=2;continue;}
        }

        // 单字符
        switch(c){
        case '+':push(TK::PLUS,"+");break; case '-':push(TK::MINUS,"-");break;
        case '*':push(TK::STAR,"*");break; case '/':push(TK::SLASH,"/");break;
        case '%':push(TK::PERCENT,"%");break; case '^':push(TK::CARET,"^");break;
        case '<':push(TK::LT,"<");break;   case '>':push(TK::GT,">");break;
        case '=':push(TK::ASSIGN,"=");break;
        case ',':push(TK::COMMA,",");break; case '(':push(TK::LPAREN,"(");break;
        case ')':push(TK::RPAREN,")");break;case '[':push(TK::LBRACKET,"[");break;
        case ']':push(TK::RBRACKET,"]");break;case '{':push(TK::LBRACE,"{");break;
        case '}':push(TK::RBRACE,"}");break;case '.':push(TK::DOT,".");break;
        case ':':push(TK::COLON,":");break; case ';':push(TK::SEMICOLON,";");break;
        case '#':push(TK::HASH,"#");break;
        default: push(TK::UNKNOWN,std::string(1,c)); break;
        }
        ++i;
    }
    push(TK::END,"");
}

// ============================================================================
//  辅助
// ============================================================================
const LogitechParser::Token& LogitechParser::peek(int offset) const {
    size_t idx = m_pos + (size_t)offset;
    return idx < m_tokens.size() ? m_tokens[idx] : kEOF;
}
LogitechParser::Token LogitechParser::consume() {
    return m_pos < m_tokens.size() ? m_tokens[m_pos++] : kEOF;
}
bool LogitechParser::check(TK t) const { return peek().type == t; }
bool LogitechParser::checkKw(const char* kw) const {
    if (!check(TK::IDENT)) return false;
    std::string lo = peek().value, ref(kw);
    std::transform(lo.begin(),lo.end(),lo.begin(),[](unsigned char c){return (char)std::tolower(c);});
    std::transform(ref.begin(),ref.end(),ref.begin(),[](unsigned char c){return (char)std::tolower(c);});
    return lo == ref;
}
void LogitechParser::skipNL() { while(check(TK::NEWLINE)||check(TK::SEMICOLON)) consume(); }
void LogitechParser::addWarn(int ln, const std::string& msg) {
    m_diags.push_back({DiagLevel::Warning, ln, msg, {}});
}
void LogitechParser::addError(const std::string& msg) {
    m_diags.push_back({DiagLevel::Error, 0, msg, {}});
}
bool LogitechParser::isBlockEnd() const {
    if (check(TK::END)) return true;
    if (!check(TK::IDENT)) return false;
    std::string lo = peek().value;
    std::transform(lo.begin(),lo.end(),lo.begin(),[](unsigned char c){return (char)std::tolower(c);});
    return lo=="end"||lo=="else"||lo=="elseif"||lo=="until";
}

// ============================================================================
//  解析入口
// ============================================================================
AIRNodePtr LogitechParser::parse() { return parseProgram(); }

AIRNodePtr LogitechParser::parseProgram() {
    auto prog = ProgramNode::make();
    skipNL();
    while (!check(TK::END)) {
        auto n = parseTopLevel();
        if (n) prog->addChild(std::move(n));
        skipNL();
    }
    return prog;
}

AIRNodePtr LogitechParser::parseTopLevel() {
    if (check(TK::IDENT) && peek().value.substr(0,7)=="__CMT__") return parseComment();
    if (checkKw("function")) return parseFuncDef();
    // 全局赋值 / 调用
    if (check(TK::IDENT)) return parseAssignOrCall();
    // 跳过无法识别的行
    int ln = peek().line;
    std::string raw;
    while (!check(TK::NEWLINE) && !check(TK::END)) raw += consume().value + " ";
    if (!raw.empty()) addWarn(ln, "顶层无法解析: " + raw);
    return raw.empty() ? nullptr : RawNode::make(raw);
}

// ── function Name(params) ... end ────────────────────────────────────────────
AIRNodePtr LogitechParser::parseFuncDef() {
    consume(); // function
    std::string name;
    if (check(TK::IDENT)) name = consume().value;
    // 方法语法：Module:Method → 合并为 Module_Method
    if (check(TK::COLON) || check(TK::DOT)) {
        consume();
        if (check(TK::IDENT)) name += "_" + consume().value;
    }

    if (check(TK::LPAREN)) consume();
    std::vector<std::string> params;
    while (!check(TK::RPAREN) && !check(TK::END)) {
        if (check(TK::IDENT)) params.push_back(consume().value);
        if (check(TK::COMMA)) consume();
    }
    if (check(TK::RPAREN)) consume();

    // OnEvent 特殊处理：生成 TriggerNode 框架
    // 结构：OnEvent(event, arg) → 解析函数体，把 if event == "X" and arg == N 提取为 TriggerNode
    // 为简化，直接把整个函数体解析为一个序列，套在 TriggerNode(通配) 中
    skipNL();
    auto body = parseBlock();

    // 如果是 OnEvent，用触发器包裹
    std::string nameLo = name;
    std::transform(nameLo.begin(),nameLo.end(),nameLo.begin(),[](unsigned char c){return (char)std::tolower(c);});
    if (nameLo == "onevent") {
        auto prog_seq = SequenceNode::make();
        prog_seq->addChild(CommentNode::make("OnEvent 触发器 - 请按需修改触发键"));
        prog_seq->addChild(std::move(body));
        auto trigger = TriggerNode::make(TriggerEvent::MouseButtonDown,
                                         AIRKey::KEY_MOUSE_LEFT, ModMask::None);
        trigger->addChild(std::move(prog_seq));
        if (checkKw("end")) consume();
        return trigger;
    }

    auto fd = FunctionDefNode::make(name, params);
    fd->addChild(std::move(body));
    if (checkKw("end")) consume();
    return fd;
}

// ── 语句块（遇到 end/else/elseif/until/EOF 停止）────────────────────────────
AIRNodePtr LogitechParser::parseBlock() {
    auto seq = SequenceNode::make();
    skipNL();
    while (!isBlockEnd() && !check(TK::END)) {
        auto st = parseStatement();
        if (st) seq->addChild(std::move(st));
        skipNL();
    }
    return seq;
}

// ── 单条语句 ─────────────────────────────────────────────────────────────────
AIRNodePtr LogitechParser::parseStatement() {
    while (check(TK::NEWLINE) || check(TK::SEMICOLON)) consume();
    if (isBlockEnd() || check(TK::END)) return nullptr;

    int ln = peek().line;

    if (check(TK::IDENT) && peek().value.substr(0,7)=="__CMT__") return parseComment();
    if (checkKw("local"))  return parseLocal();
    if (checkKw("if"))     return parseIf();
    if (checkKw("while"))  return parseWhile();
    if (checkKw("repeat")) return parseRepeat();
    if (checkKw("for"))    return parseFor();
    if (checkKw("break"))  { consume(); return BreakNode::make(); }
    if (checkKw("return")) return parseReturn();
    if (checkKw("do")) {   // do ... end 块
        consume(); skipNL();
        auto b = parseBlock();
        if (checkKw("end")) consume();
        return b;
    }
    if (check(TK::IDENT)) return parseAssignOrCall();

    // 无法识别
    std::string raw;
    while (!check(TK::NEWLINE) && !check(TK::END) && !isBlockEnd())
        raw += consume().value + " ";
    if (!raw.empty()) addWarn(ln, "无法解析语句: " + raw);
    return raw.empty() ? nullptr : RawNode::make(raw);
}

AIRNodePtr LogitechParser::parseComment() {
    std::string raw = consume().value;
    return CommentNode::make(raw.substr(8), false); // "__CMT__:"
}

// ── local name [= expr] ──────────────────────────────────────────────────────
AIRNodePtr LogitechParser::parseLocal() {
    consume(); // local
    std::string nm;
    if (check(TK::IDENT)) nm = consume().value;
    auto n = VarDeclareNode::make(nm, VarScope::Local);
    if (check(TK::ASSIGN)) { consume(); n->addChild(parseExpr()); }
    return n;
}

// ── name = expr  OR  name(args) ──────────────────────────────────────────────
AIRNodePtr LogitechParser::parseAssignOrCall() {
    int ln = peek().line;
    std::string name = consume().value;

    // 函数调用
    if (check(TK::LPAREN)) {
        consume();
        return parseApiCall(name, ln);
    }
    // 方法调用 obj:method(args) 或 obj.method(args)
    if ((check(TK::COLON) || check(TK::DOT)) &&
        m_pos+1 < m_tokens.size() && m_tokens[m_pos+1].type==TK::IDENT) {
        std::string sep = consume().value;
        std::string method = consume().value;
        if (check(TK::LPAREN)) { consume(); return parseApiCall(name+"_"+method, ln); }
    }
    // 赋值
    if (check(TK::ASSIGN)) {
        consume();
        auto n = VarAssignNode::make(name);
        n->addChild(parseExpr());
        return n;
    }
    // 只有标识符（可能是语句表达式，忽略）
    addWarn(ln, "无法解析的语句标识符: " + name);
    return RawNode::make(name);
}

// ── 罗技 API 调用（已消耗 '('）───────────────────────────────────────────────
AIRNodePtr LogitechParser::parseApiCall(const std::string& rawName, int ln) {
    // 函数名小写化便于匹配
    std::string nm = rawName;
    std::transform(nm.begin(),nm.end(),nm.begin(),[](unsigned char c){return (char)std::tolower(c);});

    auto args = parseArgList(); // 消耗到 ')'

    // ── Sleep(ms) ─────────────────────────────────────────────────────────
    if (nm == "sleep") {
        auto n = SleepNode::make(0); n->children.clear();
        n->addChild(args.empty() ? ExprNumberNode::make(0) : std::move(args[0]));
        return n;
    }
    // ── PressKey("key") ───────────────────────────────────────────────────
    if (nm == "presskey") {
        std::string k = args.empty() ? "" :
            (args[0]->type==AIRNodeType::ExprString ? static_cast<ExprStringNode*>(args[0].get())->value : "");
        AIRKey key = LogitechKeyMap::LuaKeyToAIR(k);
        return KeyDownNode::make(key, k);
    }
    // ── ReleaseKey("key") ─────────────────────────────────────────────────
    if (nm == "releasekey") {
        std::string k = args.empty() ? "" :
            (args[0]->type==AIRNodeType::ExprString ? static_cast<ExprStringNode*>(args[0].get())->value : "");
        AIRKey key = LogitechKeyMap::LuaKeyToAIR(k);
        return KeyUpNode::make(key, k);
    }
    // ── PressAndReleaseKey("key") ─────────────────────────────────────────
    if (nm == "pressandreleasekey") {
        std::string k = args.empty() ? "" :
            (args[0]->type==AIRNodeType::ExprString ? static_cast<ExprStringNode*>(args[0].get())->value : "");
        AIRKey key = LogitechKeyMap::LuaKeyToAIR(k);
        return KeyTapNode::make(key, 1, 0, k);
    }
    // ── PressMouseButton(btn) ─────────────────────────────────────────────
    if (nm == "pressmousebutton") {
        int btn = 1;
        if (!args.empty() && args[0]->type==AIRNodeType::ExprNumber)
            btn = (int)static_cast<ExprNumberNode*>(args[0].get())->value;
        return MouseDownNode::make(LogitechKeyMap::LuaMouseBtnToAIR(btn));
    }
    // ── ReleaseMouseButton(btn) ───────────────────────────────────────────
    if (nm == "releasemousebutton") {
        int btn = 1;
        if (!args.empty() && args[0]->type==AIRNodeType::ExprNumber)
            btn = (int)static_cast<ExprNumberNode*>(args[0].get())->value;
        return MouseUpNode::make(LogitechKeyMap::LuaMouseBtnToAIR(btn));
    }
    // ── MoveMouseRelative(dx, dy) ─────────────────────────────────────────
    if (nm == "movemouserelative") {
        auto n = MouseMoveNode::make(CoordType::CoordRelative);
        n->addChild(args.size()>0 ? std::move(args[0]) : ExprNumberNode::make(0));
        n->addChild(args.size()>1 ? std::move(args[1]) : ExprNumberNode::make(0));
        return n;
    }
    // ── MoveMouseTo(x, y) ────────────────────────────────────────────────
    if (nm == "movemouseto") {
        auto n = MouseMoveNode::make(CoordType::CoordAbsolute);
        n->addChild(args.size()>0 ? std::move(args[0]) : ExprNumberNode::make(0));
        n->addChild(args.size()>1 ? std::move(args[1]) : ExprNumberNode::make(0));
        return n;
    }
    // ── MoveMouseWheel(clicks) ────────────────────────────────────────────
    if (nm == "movemousewheel") {
        // 正数=向上，负数=向下
        int clicks = 1;
        if (!args.empty() && args[0]->type==AIRNodeType::ExprNumber)
            clicks = (int)static_cast<ExprNumberNode*>(args[0].get())->value;
        auto n = MouseScrollNode::make(clicks >= 0 ? ScrollDir::Up : ScrollDir::Down);
        n->addChild(ExprNumberNode::make(std::abs(clicks)));
        return n;
    }
    // ── IsMouseButtonPressed(btn) → 作为语句调用时忽略返回值 ─────────────
    if (nm == "ismousebuttonpressed") {
        // 作为语句调用很少见，一般用于条件，返回 QueryKeyStateNode
        int btn = 1;
        if (!args.empty() && args[0]->type==AIRNodeType::ExprNumber)
            btn = (int)static_cast<ExprNumberNode*>(args[0].get())->value;
        return QueryKeyStateNode::make(LogitechKeyMap::LuaMouseBtnToAIR(btn));
    }

    // ── 其他：普通函数调用 ────────────────────────────────────────────────
    auto n = FunctionCallNode::make(rawName);
    for (auto& a : args) n->addChild(std::move(a));
    return n;
}

std::vector<AIRNodePtr> LogitechParser::parseArgList() {
    std::vector<AIRNodePtr> args;
    while (!check(TK::RPAREN) && !check(TK::END)) {
        args.push_back(parseExpr());
        if (check(TK::COMMA)) consume();
    }
    if (check(TK::RPAREN)) consume();
    return args;
}

// ── if ... then ... [elseif...] [else...] end ────────────────────────────────
AIRNodePtr LogitechParser::parseIf() {
    consume(); // if
    auto n = IfNode::make();
    n->addChild(parseExpr());
    if (checkKw("then")) consume();
    skipNL();
    n->addChild(parseBlock());

    while (checkKw("elseif")) {
        // elseif 链：展开为嵌套 IfNode
        consume();
        auto inner = IfNode::make();
        inner->addChild(parseExpr());
        if (checkKw("then")) consume();
        skipNL();
        inner->addChild(parseBlock());
        // 继续收集 elseif/else/end
        if (checkKw("else")) {
            consume(); skipNL();
            inner->addChild(parseBlock());
        }
        if (checkKw("end")) consume();
        // 把 inner 作为当前 n 的 else 分支
        n->addChild(std::move(inner));
        return n; // 已处理完
    }
    if (checkKw("else")) { consume(); skipNL(); n->addChild(parseBlock()); }
    if (checkKw("end")) consume();
    return n;
}

// ── while cond do ... end ────────────────────────────────────────────────────
AIRNodePtr LogitechParser::parseWhile() {
    consume(); // while
    auto n = LoopWhileNode::make();
    n->addChild(parseExpr());
    if (checkKw("do")) consume();
    skipNL();
    n->addChild(parseBlock());
    if (checkKw("end")) consume();
    return n;
}

// ── repeat ... until cond ────────────────────────────────────────────────────
AIRNodePtr LogitechParser::parseRepeat() {
    consume(); // repeat
    skipNL();
    auto n = LoopDoWhileNode::make();
    n->addChild(parseBlock());           // children[0]: 循环体
    if (checkKw("until")) consume();
    n->addChild(parseExpr());            // children[1]: 退出条件（为真时退出）
    return n;
}

// ── for i = start, stop [, step] do ... end ──────────────────────────────────
AIRNodePtr LogitechParser::parseFor() {
    consume(); // for
    // 数值 for
    std::string var;
    if (check(TK::IDENT)) var = consume().value;
    if (check(TK::ASSIGN)) {
        consume();
        auto start = parseExpr();
        AIRNodePtr stop, step;
        if (check(TK::COMMA)) { consume(); stop = parseExpr(); }
        if (check(TK::COMMA)) { consume(); step = parseExpr(); }
        if (checkKw("do")) consume();
        skipNL();
        auto body = parseBlock();
        if (checkKw("end")) consume();

        // 转换为 LoopCountNode（近似：用 stop - start + 1 作为次数）
        // 由于表达式可能是变量，这里只能尽力而为
        auto lc = LoopCountNode::make();
        if (stop) lc->addChild(std::move(stop));
        else lc->addChild(ExprNumberNode::make(1));
        lc->addChild(std::move(body));
        return lc;
    }
    // 泛型 for（for k,v in pairs(t)）→ 包装为 RawNode
    addWarn(peek().line, "泛型 for 不支持，已跳过");
    while (!checkKw("end") && !check(TK::END)) {
        if (checkKw("do")) consume();
        consume();
    }
    if (checkKw("end")) consume();
    return RawNode::make("-- for loop skipped");
}

// ── return [expr] ────────────────────────────────────────────────────────────
AIRNodePtr LogitechParser::parseReturn() {
    consume(); // return
    auto n = ReturnNode::make();
    if (!check(TK::NEWLINE) && !check(TK::END) && !isBlockEnd())
        n->addChild(parseExpr());
    return n;
}

// ============================================================================
//  表达式（Lua 运算符优先级）
// ============================================================================
AIRNodePtr LogitechParser::parseExpr()    { return parseOrExpr(); }

AIRNodePtr LogitechParser::parseOrExpr() {
    auto l = parseAndExpr();
    while (checkKw("or")) {
        consume();
        auto n = ExprBinopNode::make(BinOp::OpOr);
        n->addChild(std::move(l)); n->addChild(parseAndExpr()); l = std::move(n);
    }
    return l;
}

AIRNodePtr LogitechParser::parseAndExpr() {
    auto l = parseNotExpr();
    while (checkKw("and")) {
        consume();
        auto n = ExprBinopNode::make(BinOp::OpAnd);
        n->addChild(std::move(l)); n->addChild(parseNotExpr()); l = std::move(n);
    }
    return l;
}

AIRNodePtr LogitechParser::parseNotExpr() {
    if (checkKw("not")) {
        consume();
        auto n = ExprUnopNode::make(UnOp::OpNot);
        n->addChild(parseNotExpr()); return n;
    }
    return parseCompare();
}

AIRNodePtr LogitechParser::parseCompare() {
    auto l = parseConcat();
    while (check(TK::EQ2)||check(TK::NEQ)||check(TK::LT)||check(TK::GT)||check(TK::LTE)||check(TK::GTE)) {
        BinOp op;
        switch(peek().type){
        case TK::EQ2: op=BinOp::Eq;  break; case TK::NEQ: op=BinOp::Neq; break;
        case TK::LT:  op=BinOp::Lt;  break; case TK::GT:  op=BinOp::Gt;  break;
        case TK::LTE: op=BinOp::Lte; break; default: op=BinOp::Gte; break;
        }
        consume();
        auto n = ExprBinopNode::make(op);
        n->addChild(std::move(l)); n->addChild(parseConcat()); l = std::move(n);
    }
    return l;
}

AIRNodePtr LogitechParser::parseConcat() {
    auto l = parseAddSub();
    while (check(TK::DOTDOT)) {
        consume();
        auto n = ExprBinopNode::make(BinOp::Concat);
        n->addChild(std::move(l)); n->addChild(parseAddSub()); l = std::move(n);
    }
    return l;
}

AIRNodePtr LogitechParser::parseAddSub() {
    auto l = parseMulDiv();
    while (check(TK::PLUS)||check(TK::MINUS)) {
        BinOp op = check(TK::PLUS)?BinOp::Add:BinOp::Sub; consume();
        auto n = ExprBinopNode::make(op);
        n->addChild(std::move(l)); n->addChild(parseMulDiv()); l = std::move(n);
    }
    return l;
}

AIRNodePtr LogitechParser::parseMulDiv() {
    auto l = parseUnary();
    while (check(TK::STAR)||check(TK::SLASH)||check(TK::PERCENT)) {
        BinOp op = check(TK::STAR)?BinOp::Mul:check(TK::SLASH)?BinOp::Div:BinOp::OpMod; consume();
        auto n = ExprBinopNode::make(op);
        n->addChild(std::move(l)); n->addChild(parseUnary()); l = std::move(n);
    }
    return l;
}

AIRNodePtr LogitechParser::parseUnary() {
    if (check(TK::MINUS)) { consume(); auto n=ExprUnopNode::make(UnOp::Neg); n->addChild(parsePower()); return n; }
    if (check(TK::HASH))  { consume(); return parsePower(); } // # 长度运算符，简单忽略
    return parsePower();
}

AIRNodePtr LogitechParser::parsePower() {
    auto l = parsePrimary();
    if (check(TK::CARET)) {
        consume();
        auto n = ExprBinopNode::make(BinOp::Pow);
        n->addChild(std::move(l)); n->addChild(parseUnary()); return n;
    }
    return l;
}

AIRNodePtr LogitechParser::parsePrimary() {
    int ln = peek().line;

    if (check(TK::NUMBER)) { double v=std::stod(peek().value); consume(); return ExprNumberNode::make(v); }
    if (check(TK::STRING)) { std::string s=consume().value; return ExprStringNode::make(s); }
    if (checkKw("true"))   { consume(); return ExprBoolNode::make(true);  }
    if (checkKw("false"))  { consume(); return ExprBoolNode::make(false); }
    if (checkKw("nil"))    { consume(); return ExprBoolNode::make(false); } // nil → false

    if (check(TK::LPAREN)) {
        consume(); auto e = parseExpr();
        if (check(TK::RPAREN)) consume();
        return e;
    }

    if (check(TK::IDENT)) {
        std::string nm = peek().value;
        std::string lo = nm;
        std::transform(lo.begin(),lo.end(),lo.begin(),[](unsigned char c){return (char)std::tolower(c);});
        consume();

        // 方法/成员访问 obj.field 或 obj:method
        if ((check(TK::DOT)||check(TK::COLON)) &&
            m_pos < m_tokens.size() && m_tokens[m_pos+0].type==TK::IDENT) {
            // 简单跳过（obj.x → 变量 obj_x）
            // 如果后面接 (，当作函数调用
        }

        // 函数调用 name(args)
        if (check(TK::LPAREN)) {
            consume();

            // IsMouseButtonPressed(btn) → QueryKeyStateNode
            if (lo == "ismousebuttonpressed") {
                auto argvec = parseArgList();
                int btn = 1;
                if (!argvec.empty() && argvec[0]->type==AIRNodeType::ExprNumber)
                    btn = (int)static_cast<ExprNumberNode*>(argvec[0].get())->value;
                return QueryKeyStateNode::make(LogitechKeyMap::LuaMouseBtnToAIR(btn));
            }
            // math.random(min, max) → ExprRandomRangeNode
            if (lo == "math_random" || lo == "random") {
                auto argvec = parseArgList();
                if (argvec.size() >= 2) {
                    auto n = std::make_unique<ExprRandomRangeNode>();
                    n->addChild(std::move(argvec[0]));
                    n->addChild(std::move(argvec[1]));
                    return n;
                }
                auto n = ExprCallNode::make(nm);
                for (auto& a : argvec) n->addChild(std::move(a));
                return n;
            }

            // 普通函数调用（表达式形式）
            auto argvec = parseArgList();
            auto n = ExprCallNode::make(nm);
            for (auto& a : argvec) n->addChild(std::move(a));
            return n;
        }

        // 方法访问 obj:method() 或 obj.method()
        if (check(TK::DOT) || check(TK::COLON)) {
            consume();
            if (check(TK::IDENT)) {
                std::string method = consume().value;
                std::string mlo = method;
                std::transform(mlo.begin(),mlo.end(),mlo.begin(),[](unsigned char c){return (char)std::tolower(c);});
                if (check(TK::LPAREN)) {
                    consume();
                    // math.random
                    if (mlo == "random") {
                        auto argvec = parseArgList();
                        if (argvec.size() >= 2) {
                            auto n = std::make_unique<ExprRandomRangeNode>();
                            n->addChild(std::move(argvec[0]));
                            n->addChild(std::move(argvec[1]));
                            return n;
                        }
                    }
                    auto argvec = parseArgList();
                    auto n = ExprCallNode::make(nm+"_"+method);
                    for (auto& a : argvec) n->addChild(std::move(a));
                    return n;
                }
                // 成员访问（如 table.key）→ 变量
                return ExprVarNode::make(nm+"_"+method);
            }
        }

        return ExprVarNode::make(nm);
    }

    addWarn(ln, "无法解析表达式 token: " + peek().value);
    consume();
    return ExprNumberNode::make(0);
}

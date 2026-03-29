#include "XMouseParser.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

using namespace AIR;

XMouseParser::XMouseParser(const std::string& source, std::vector<AIRDiagnostic>& diags)
    : m_source(source), m_diags(diags) {}

AIRNodePtr XMouseParser::parse()
{
    auto program = ProgramNode::make();
    program->addChild(CommentNode::make("X-Mouse profile (subset) -> AIR"));

    auto hotkey = HotkeyNode::make(AIRKey::KEY_MOUSE_X1, ModMask::None, true);
    auto seq = SequenceNode::make();

    std::istringstream iss(m_source);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("<Button") == std::string::npos) {
            continue;
        }
        auto node = parseButtonLine(line);
        if (node) {
            seq->addChild(std::move(node));
        }
    }

    hotkey->addChild(std::move(seq));
    program->addChild(std::move(hotkey));
    return program;
}

AIRNodePtr XMouseParser::parseButtonLine(const std::string& line)
{
    std::string name = attr(line, "Name");
    std::string action = attr(line, "Action");
    std::string keys = attr(line, "Keys");

    if (action.empty()) action = attr(line, "Type");
    if (keys.empty()) keys = attr(line, "Data");

    std::transform(action.begin(), action.end(), action.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });

    auto seq = SequenceNode::make();
    if (!name.empty()) {
        seq->addChild(CommentNode::make("Button: " + name));
    }

    if (action == "simulatedkeys" || action == "simulated keystrokes") {
        auto keysNode = parseSimulatedKeys(keys);
        if (keysNode) seq->addChild(std::move(keysNode));
    } else if (action == "left click") {
        seq->addChild(MouseClickNode::make(AIRKey::KEY_MOUSE_LEFT, 1));
    } else if (action == "right click") {
        seq->addChild(MouseClickNode::make(AIRKey::KEY_MOUSE_RIGHT, 1));
    } else if (action.empty()) {
        warn("检测到 <Button> 但缺少 Action/Type，已跳过");
        return nullptr;
    } else {
        warn("不支持的 X-Mouse Action: " + action + "，已写入注释");
        seq->addChild(CommentNode::make("Unsupported XMouse action: " + action));
    }

    if (seq->children.empty()) return nullptr;
    if (seq->children.size() == 1) return std::move(seq->children[0]);
    return seq;
}

AIRNodePtr XMouseParser::parseSimulatedKeys(const std::string& keysExpr)
{
    auto seq = SequenceNode::make();
    std::string expr = trim(keysExpr);
    for (size_t i = 0; i < expr.size();) {
        if (expr[i] == '{') {
            size_t end = expr.find('}', i + 1);
            if (end == std::string::npos) break;
            std::string token = expr.substr(i + 1, end - i - 1);
            std::string upper = token;
            std::transform(upper.begin(), upper.end(), upper.begin(),
                           [](unsigned char c) { return (char)std::toupper(c); });

            const std::string waitPrefix = "WAITMS:";
            if (upper.rfind(waitPrefix, 0) == 0) {
                std::string ms = upper.substr(waitPrefix.size());
                if (isNumber(ms)) seq->addChild(SleepNode::make(std::stoi(ms)));
            } else if (upper == "LBUTTON") {
                seq->addChild(MouseClickNode::make(AIRKey::KEY_MOUSE_LEFT, 1));
            } else if (upper == "RBUTTON") {
                seq->addChild(MouseClickNode::make(AIRKey::KEY_MOUSE_RIGHT, 1));
            } else if (upper == "MBUTTON") {
                seq->addChild(MouseClickNode::make(AIRKey::KEY_MOUSE_MIDDLE, 1));
            } else {
                AIRKey key = tokenToKey(upper);
                if (key != AIRKey::KEY_UNKNOWN) {
                    seq->addChild(KeyTapNode::make(key));
                } else {
                    warn("未知模拟按键标记: {" + token + "}");
                }
            }
            i = end + 1;
            continue;
        }

        unsigned char ch = (unsigned char)expr[i];
        if (std::isalnum(ch)) {
            std::string one(1, (char)std::toupper(ch));
            AIRKey key = tokenToKey(one);
            if (key != AIRKey::KEY_UNKNOWN) {
                seq->addChild(KeyTapNode::make(key, 1, 0, one));
            }
        }
        ++i;
    }

    if (seq->children.empty()) return nullptr;
    if (seq->children.size() == 1) return std::move(seq->children[0]);
    return seq;
}

std::string XMouseParser::attr(const std::string& line, const std::string& key)
{
    std::regex re(key + R"re(\s*=\s*"([^"]*)")re", std::regex_constants::icase);
    std::smatch m;
    return std::regex_search(line, m, re) ? m[1].str() : "";
}

std::string XMouseParser::trim(std::string s)
{
    while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
    return s;
}

bool XMouseParser::isNumber(const std::string& s)
{
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}

AIRKey XMouseParser::tokenToKey(const std::string& token)
{
    if (token == "CTRL" || token == "LCTRL" || token == "RCTRL") return AIRKey::KEY_CTRL;
    if (token == "SHIFT" || token == "LSHIFT" || token == "RSHIFT") return AIRKey::KEY_SHIFT;
    if (token == "ALT" || token == "LALT" || token == "RALT") return AIRKey::KEY_ALT;
    if (token == "WIN" || token == "LWIN" || token == "RWIN") return AIRKey::KEY_WIN;
    if (token == "ENTER" || token == "RETURN") return AIRKey::KEY_ENTER;
    if (token == "TAB") return AIRKey::KEY_TAB;
    if (token == "ESC" || token == "ESCAPE") return AIRKey::KEY_ESCAPE;
    if (token.size() == 1 && token[0] >= 'A' && token[0] <= 'Z') return (AIRKey)((int)AIRKey::KEY_A + (token[0] - 'A'));
    if (token.size() == 1 && token[0] >= '0' && token[0] <= '9') return (AIRKey)((int)AIRKey::KEY_0 + (token[0] - '0'));
    return AIRKey::KEY_UNKNOWN;
}

void XMouseParser::warn(const std::string& msg) { m_diags.push_back({ DiagLevel::Warning, 0, msg, {} }); }
void XMouseParser::error(const std::string& msg) { m_diags.push_back({ DiagLevel::Error, 0, msg, {} }); }

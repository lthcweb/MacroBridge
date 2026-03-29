#include "XMouseGenerator.h"

#include <sstream>

using namespace AIR;

XMouseGenerator::XMouseGenerator(std::vector<AIRDiagnostic>& diags) : m_diags(diags) {}

std::string XMouseGenerator::generate(const AIRNode& root)
{
    std::string keys;
    emitNode(root, keys);

    std::ostringstream oss;
    oss << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    oss << "<XMouseProfiles>\n";
    oss << "  <Profile Name=\"MacroBridge_XMouse\">\n";
    oss << "    <Layer Index=\"1\">\n";
    oss << "      <Button Name=\"XButton1\" Action=\"SimulatedKeys\" Keys=\"" << keys << "\"/>\n";
    oss << "    </Layer>\n";
    oss << "  </Profile>\n";
    oss << "</XMouseProfiles>\n";
    return oss.str();
}

void XMouseGenerator::emitNode(const AIRNode& node, std::string& out)
{
    switch (node.type) {
    case AIRNodeType::Program:
    case AIRNodeType::Sequence:
    case AIRNodeType::Hotkey:
    case AIRNodeType::Trigger:
        for (const auto& ch : node.children) emitNode(*ch, out);
        break;
    case AIRNodeType::Sleep: {
        const auto& s = static_cast<const SleepNode&>(node);
        int ms = 0;
        if (!s.children.empty()) {
            if (auto* n = dynamic_cast<ExprNumberNode*>(s.children[0].get())) {
                ms = static_cast<int>(n->value);
            }
        }
        out += "{WAITMS:" + std::to_string(ms) + "}";
        break;
    }
    case AIRNodeType::KeyTap: {
        const auto& k = static_cast<const KeyTapNode&>(node);
        out += keyToToken(k.key, k.rawKey);
        break;
    }
    case AIRNodeType::KeyDown: {
        const auto& k = static_cast<const KeyDownNode&>(node);
        out += keyToToken(k.key, k.rawKey);
        break;
    }
    case AIRNodeType::KeyUp: {
        const auto& k = static_cast<const KeyUpNode&>(node);
        out += keyToToken(k.key, k.rawKey);
        break;
    }
    case AIRNodeType::MouseClick: {
        const auto& m = static_cast<const MouseClickNode&>(node);
        if (m.key == AIRKey::KEY_MOUSE_LEFT) out += "{LBUTTON}";
        else if (m.key == AIRKey::KEY_MOUSE_RIGHT) out += "{RBUTTON}";
        else if (m.key == AIRKey::KEY_MOUSE_MIDDLE) out += "{MBUTTON}";
        break;
    }
    default:
        for (const auto& ch : node.children) emitNode(*ch, out);
        break;
    }
}

std::string XMouseGenerator::keyToToken(AIRKey key, const std::string& raw)
{
    if (!raw.empty()) return "{" + raw + "}";
    if (key >= AIRKey::KEY_A && key <= AIRKey::KEY_Z) {
        char c = static_cast<char>('A' + (int(key) - int(AIRKey::KEY_A)));
        return std::string(1, c);
    }
    if (key >= AIRKey::KEY_0 && key <= AIRKey::KEY_9) {
        char c = static_cast<char>('0' + (int(key) - int(AIRKey::KEY_0)));
        return std::string(1, c);
    }
    if (key == AIRKey::KEY_CTRL) return "{CTRL}";
    if (key == AIRKey::KEY_SHIFT) return "{SHIFT}";
    if (key == AIRKey::KEY_ALT) return "{ALT}";
    if (key == AIRKey::KEY_WIN) return "{WIN}";
    if (key == AIRKey::KEY_ENTER) return "{ENTER}";
    if (key == AIRKey::KEY_TAB) return "{TAB}";
    if (key == AIRKey::KEY_ESCAPE) return "{ESC}";
    return "{UNKNOWN}";
}

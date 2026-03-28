/*
 * RazerParser.cpp  —  雷蛇 XML 宏 → AIR 树解析器实现
 */

#include "RazerParser.h"
#include "RazerKeyMap.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sstream>

using namespace AIR;

// ============================================================================
//  构造
// ============================================================================

RazerParser::RazerParser(const std::string& xmlSource,
                         std::vector<AIRDiagnostic>& diags,
                         RazerSynapseVersion version)
    : m_source(xmlSource), m_diags(diags), m_version(version)
{
}

// ============================================================================
//  §1  XmlNode 辅助方法
// ============================================================================

const RazerParser::XmlNode*
RazerParser::XmlNode::child(const std::string& tagName) const
{
    for (const auto& c : children)
        if (c.tag == tagName) return &c;
    return nullptr;
}

std::vector<const RazerParser::XmlNode*>
RazerParser::XmlNode::children_of(const std::string& tagName) const
{
    std::vector<const XmlNode*> result;
    for (const auto& c : children)
        if (c.tag == tagName) result.push_back(&c);
    return result;
}

std::string RazerParser::XmlNode::text_trimmed() const
{
    size_t b = text.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    size_t e = text.find_last_not_of(" \t\r\n");
    return text.substr(b, e - b + 1);
}

std::string RazerParser::XmlNode::child_text(
    const std::string& tagName,
    const std::string& defaultVal) const
{
    const XmlNode* c = child(tagName);
    return c ? c->text_trimmed() : defaultVal;
}

int RazerParser::XmlNode::child_int(
    const std::string& tagName,
    int defaultVal) const
{
    std::string t = child_text(tagName);
    if (t.empty()) return defaultVal;
    try { return std::stoi(t); }
    catch (...) { return defaultVal; }
}

// ============================================================================
//  §2  轻量 XML 解析器
//
//  支持：
//    - 元素嵌套
//    - 文本内容
//    - 属性（忽略属性值，只解析元素）
//    - <!-- --> 注释（跳过）
//    - <?xml ...?> 声明（跳过）
//    - CDATA 块（作为文本处理）
//    - 自闭合标签 <Tag/>
//  不支持（雷蛇宏中不出现）：
//    - 命名空间
//    - DTD
// ============================================================================

void RazerParser::skipWhitespace(size_t& pos) const
{
    while (pos < m_source.size() &&
           (m_source[pos] == ' '  || m_source[pos] == '\t' ||
            m_source[pos] == '\r' || m_source[pos] == '\n'))
        ++pos;
}

void RazerParser::skipComment(size_t& pos) const
{
    // pos 指向 <!-- 的 <，已确认接下来是 <!--
    pos += 4; // 跳过 <!--
    while (pos + 2 < m_source.size()) {
        if (m_source[pos] == '-' && m_source[pos+1] == '-' &&
            m_source[pos+2] == '>') {
            pos += 3;
            return;
        }
        ++pos;
    }
}

void RazerParser::skipDeclaration(size_t& pos) const
{
    // pos 指向 <?，跳到 ?>
    pos += 2;
    while (pos + 1 < m_source.size()) {
        if (m_source[pos] == '?' && m_source[pos+1] == '>') {
            pos += 2;
            return;
        }
        ++pos;
    }
}

std::string RazerParser::parseTagName(size_t& pos) const
{
    size_t start = pos;
    while (pos < m_source.size() &&
           m_source[pos] != ' ' && m_source[pos] != '\t' &&
           m_source[pos] != '\r'&& m_source[pos] != '\n' &&
           m_source[pos] != '>' && m_source[pos] != '/' &&
           m_source[pos] != ':') // 跳过命名空间前缀（xmlns:xsi 等）
        ++pos;
    // 若遇到命名空间冒号，跳过前缀，重新解析本地名
    if (pos < m_source.size() && m_source[pos] == ':') {
        ++pos;
        return parseTagName(pos);
    }
    return m_source.substr(start, pos - start);
}

void RazerParser::skipAttributes(size_t& pos) const
{
    // 跳过到 > 或 />，处理引号内的 > 不被误判
    while (pos < m_source.size() && m_source[pos] != '>') {
        if (m_source[pos] == '"') {
            ++pos;
            while (pos < m_source.size() && m_source[pos] != '"') ++pos;
            if (pos < m_source.size()) ++pos;
        } else if (m_source[pos] == '\'') {
            ++pos;
            while (pos < m_source.size() && m_source[pos] != '\'') ++pos;
            if (pos < m_source.size()) ++pos;
        } else {
            ++pos;
        }
    }
}

// 解析文本内容，直到遇到 <endTag> 的结束标签（< /endTag>）
// 返回原始文本（含 XML 实体引用，此处简单处理常见实体）
std::string RazerParser::parseText(size_t& pos,
                                   const std::string& /*endTag*/) const
{
    std::string result;
    while (pos < m_source.size() && m_source[pos] != '<') {
        result += m_source[pos++];
    }
    // 处理常见 XML 实体
    auto replaceAll = [](std::string& s,
                         const std::string& from,
                         const std::string& to) {
        size_t p = 0;
        while ((p = s.find(from, p)) != std::string::npos) {
            s.replace(p, from.size(), to);
            p += to.size();
        }
    };
    replaceAll(result, "&amp;",  "&");
    replaceAll(result, "&lt;",   "<");
    replaceAll(result, "&gt;",   ">");
    replaceAll(result, "&quot;", "\"");
    replaceAll(result, "&apos;", "'");
    return result;
}

RazerParser::XmlNode RazerParser::parseElement(size_t& pos)
{
    XmlNode node;

    // 跳过空白
    skipWhitespace(pos);
    if (pos >= m_source.size()) return node;

    // 必须以 < 开头
    if (m_source[pos] != '<')
        throw std::runtime_error("期望 '<'，实际遇到: " +
                                 std::string(1, m_source[pos]));
    ++pos;

    // 跳过注释
    if (pos + 2 < m_source.size() &&
        m_source[pos] == '!' && m_source[pos+1] == '-') {
        skipComment(pos);
        return parseElement(pos);
    }

    // 跳过 XML 声明
    if (pos < m_source.size() && m_source[pos] == '?') {
        skipDeclaration(pos);
        return parseElement(pos);
    }

    // 跳过 <!DOCTYPE 等
    if (pos < m_source.size() && m_source[pos] == '!') {
        while (pos < m_source.size() && m_source[pos] != '>') ++pos;
        if (pos < m_source.size()) ++pos;
        return parseElement(pos);
    }

    // 解析标签名
    skipWhitespace(pos);
    node.tag = parseTagName(pos);

    // 跳过属性
    skipAttributes(pos);

    // 自闭合标签 <Tag/>
    if (pos < m_source.size() && m_source[pos-1] == '/') {
        if (pos < m_source.size() && m_source[pos] == '>') ++pos;
        return node;
    }
    if (pos < m_source.size() && m_source[pos] == '>') ++pos;

    // 解析子元素和文本内容
    while (pos < m_source.size()) {
        skipWhitespace(pos);
        if (pos >= m_source.size()) break;

        if (m_source[pos] == '<') {
            // 结束标签：</Tag>
            if (pos + 1 < m_source.size() && m_source[pos+1] == '/') {
                pos += 2; // 跳过 </
                // 跳过标签名和 >
                while (pos < m_source.size() && m_source[pos] != '>') ++pos;
                if (pos < m_source.size()) ++pos;
                break;
            }
            // CDATA：<![CDATA[...]]>
            if (pos + 8 < m_source.size() &&
                m_source.substr(pos, 9) == "<![CDATA[") {
                pos += 9;
                size_t cdataEnd = m_source.find("]]>", pos);
                if (cdataEnd != std::string::npos) {
                    node.text += m_source.substr(pos, cdataEnd - pos);
                    pos = cdataEnd + 3;
                }
                continue;
            }
            // 注释
            if (pos + 3 < m_source.size() &&
                m_source.substr(pos, 4) == "<!--") {
                skipComment(pos);
                continue;
            }
            // 子元素
            XmlNode child = parseElement(pos);
            if (!child.tag.empty())
                node.children.push_back(std::move(child));
        } else {
            // 文本内容
            node.text += parseText(pos, node.tag);
        }
    }

    return node;
}

RazerParser::XmlNode RazerParser::parseXml()
{
    size_t pos = 0;
    return parseElement(pos);
}

// ============================================================================
//  §3  诊断辅助
// ============================================================================

void RazerParser::warn(const std::string& msg, const std::string& suggestion)
{
    m_diags.push_back({ DiagLevel::Warning, 0, msg, suggestion });
}

void RazerParser::error(const std::string& msg)
{
    m_diags.push_back({ DiagLevel::Error, 0, msg, {} });
}

// ============================================================================
//  §4  解析入口
// ============================================================================

AIRNodePtr RazerParser::parse()
{
    XmlNode root;
    try {
        root = parseXml();
    } catch (const std::exception& e) {
        error(std::string("XML 解析失败: ") + e.what());
        return nullptr;
    }

    // 找到根节点（Synapse 3/4 根标签不同）
    const XmlNode* macro = &root;
    if (root.tag != "Macro" && root.tag != "RazerMacro") {
        if (m_version == RazerSynapseVersion::Synapse3) {
            macro = root.child("RazerMacro");
        } else if (m_version == RazerSynapseVersion::Synapse4) {
            macro = root.child("Macro");
        } else {
            macro = root.child("Macro");
            if (!macro) macro = root.child("RazerMacro");
        }
    }

    if (!macro || (macro->tag != "Macro" && macro->tag != "RazerMacro")) {
        error("未找到有效根节点：Synapse 3 需要 <RazerMacro>，Synapse 4 需要 <Macro>");
        return nullptr;
    }

    if (m_version == RazerSynapseVersion::Synapse3 && macro->tag != "RazerMacro") {
        error("Razer3Plugin 期望根节点 <RazerMacro>，请确认是否选择了 Synapse 4 插件");
        return nullptr;
    }

    if (m_version == RazerSynapseVersion::Synapse4 && macro->tag != "Macro") {
        error("Razer4Plugin 期望根节点 <Macro>，请确认是否选择了 Synapse 3 插件");
        return nullptr;
    }

    return buildProgram(*macro);
}

// ============================================================================
//  §5  AIR 构建
// ============================================================================

AIRNodePtr RazerParser::buildProgram(const XmlNode& macroRoot)
{
    auto program = ProgramNode::make();

    // 提取宏名称，作为注释写入
    std::string macroName = macroRoot.child_text("Name", "UnknownMacro");
    program->addChild(CommentNode::make(
        "雷蛇宏: " + macroName + "  (由 RazerPlugin 转换)"));

    // 获取 MacroEvents 节点
    const XmlNode* events = macroRoot.child("MacroEvents");
    if (!events) {
        warn("未找到 <MacroEvents> 节点，宏内容为空");
        return program;
    }

    // 整体结构：用左键触发器包裹（大多数游戏宏是左键持续触发）
    // 注意：雷蛇宏不含触发器定义，触发方式由鼠标硬件决定。
    // 这里生成一个 HotkeyNode（左键热键）作为占位，用户可自行修改触发键。
    auto hotkey = HotkeyNode::make(AIRKey::KEY_MOUSE_LEFT,
                                    ModMask::None,
                                   true /* passthrough */);

    auto body = buildSequence(*events);
    if (!body) {
        return nullptr;
    }
    hotkey->addChild(std::move(body));
    program->addChild(std::move(hotkey));

    return program;
}

AIRNodePtr RazerParser::buildSequence(const XmlNode& macroEvents)
{
    auto seq = SequenceNode::make();

    for (const XmlNode& event : macroEvents.children) {
        AIRNodePtr node;

        if (m_version == RazerSynapseVersion::Synapse3) {
            if (event.tag == "MacroEvent") {
                error("Razer3Plugin 不支持 <MacroEvent> 结构，请改用 Razer4Plugin");
                return nullptr;
            }
            node = buildLegacyEvent(event);
        } else if (m_version == RazerSynapseVersion::Synapse4) {
            if (event.tag != "MacroEvent") {
                error("Razer4Plugin 仅支持 <MacroEvent> 结构，请改用 Razer3Plugin");
                return nullptr;
            }
            node = buildMacroEvent(event);
        } else {
            if (event.tag == "MacroEvent") {
                node = buildMacroEvent(event);
            } else {
                node = buildLegacyEvent(event);
            }
        }
        if (node) seq->addChild(std::move(node));
    }

    return seq;
}

AIRNodePtr RazerParser::buildMacroEvent(const XmlNode& event)
{
    int type  = event.child_int("Type",  0);
    int delay = event.child_int("Delay", 0);

    switch (type) {

    // ── Type 1：键盘按下 ──────────────────────────────────────────────────
    case 1: {
        // 先输出 delay（若有）
        // 键盘事件的 delay 表示"按下前等待"
        auto seq = SequenceNode::make();
        if (delay > 0) seq->addChild(SleepNode::make(delay));
        auto keyNode = buildKeyboardEvent(event, /*isDown=*/true);
        if (keyNode) seq->addChild(std::move(keyNode));
        // 若序列只有一个节点，直接返回
        if (seq->children.size() == 1) return std::move(seq->children[0]);
        return seq;
    }

    // ── Type 2：键盘松开 ──────────────────────────────────────────────────
    case 2: {
        auto seq = SequenceNode::make();
        if (delay > 0) seq->addChild(SleepNode::make(delay));
        auto keyNode = buildKeyboardEvent(event, /*isDown=*/false);
        if (keyNode) seq->addChild(std::move(keyNode));
        if (seq->children.size() == 1) return std::move(seq->children[0]);
        return seq;
    }

    // ── Type 3：鼠标移动序列 ──────────────────────────────────────────────
    case 3: {
        const XmlNode* movement = event.child("MouseMovement");
        if (!movement) {
            warn("Type=3 的 MacroEvent 缺少 <MouseMovement> 子节点");
            return nullptr;
        }
        return buildMouseMovement(*movement, delay);
    }

    // ── Type 4：鼠标按键按下 ──────────────────────────────────────────────
    case 4: {
        auto seq = SequenceNode::make();
        if (delay > 0) seq->addChild(SleepNode::make(delay));
        auto btnNode = buildMouseButtonEvent(event, /*isDown=*/true);
        if (btnNode) seq->addChild(std::move(btnNode));
        if (seq->children.size() == 1) return std::move(seq->children[0]);
        return seq;
    }

    // ── Type 5：鼠标按键松开 ──────────────────────────────────────────────
    case 5: {
        auto seq = SequenceNode::make();
        if (delay > 0) seq->addChild(SleepNode::make(delay));
        auto btnNode = buildMouseButtonEvent(event, /*isDown=*/false);
        if (btnNode) seq->addChild(std::move(btnNode));
        if (seq->children.size() == 1) return std::move(seq->children[0]);
        return seq;
    }

    // ── Type 6：纯延迟 ───────────────────────────────────────────────────
    case 6:
        if (delay > 0) return SleepNode::make(delay);
        return nullptr;

    // ── 未知类型 ──────────────────────────────────────────────────────────
    default:
        warn("未知的 MacroEvent Type=" + std::to_string(type) + "，已跳过",
             "请提交此宏样本以便添加支持");
        return RawNode::make("<!-- UnknownMacroEvent Type=" +
                             std::to_string(type) + " -->");
    }
}

AIRNodePtr RazerParser::buildLegacyEvent(const XmlNode& event)
{
    // 旧版样式（例如 RazerPlugin demo）：
    // <MacroEvents>
    //   <MouseButtonEvent><Type>1|2</Type><Button>1</Button></MouseButtonEvent>
    //   <MouseMovementEvent><Type>3</Type><X>..</X><Y>..</Y><Delay>..</Delay></MouseMovementEvent>
    //   <DelayEvent><Delay>..</Delay></DelayEvent>
    // </MacroEvents>

    if (event.tag == "DelayEvent") {
        int delay = event.child_int("Delay", 0);
        return delay > 0 ? SleepNode::make(delay) : nullptr;
    }

    if (event.tag == "MouseButtonEvent") {
        int type = event.child_int("Type", 0);
        int delay = event.child_int("Delay", 0);
        auto seq = SequenceNode::make();
        if (delay > 0) seq->addChild(SleepNode::make(delay));

        AIRKey key = RazerKeyMap::RazerMouseButtonToAIR(event.child_int("Button", 1));
        if (type == 1) {
            seq->addChild(MouseDownNode::make(key));
        } else if (type == 2) {
            seq->addChild(MouseUpNode::make(key));
        } else {
            warn("旧格式 MouseButtonEvent 的 Type 既不是 1(Down) 也不是 2(Up)，已跳过");
            return nullptr;
        }

        if (seq->children.size() == 1) return std::move(seq->children[0]);
        return seq;
    }

    if (event.tag == "KeyboardEvent") {
        int type = event.child_int("Type", 0);
        int delay = event.child_int("Delay", 0);
        std::string keyStr = event.child_text("Key");
        if (keyStr.empty()) {
            warn("旧格式 KeyboardEvent 缺少 <Key>，已跳过");
            return nullptr;
        }

        auto seq = SequenceNode::make();
        if (delay > 0) seq->addChild(SleepNode::make(delay));

        AIRKey key = RazerKeyMap::RazerKeyToAIR(keyStr);
        if (type == 1) {
            seq->addChild(KeyDownNode::make(key, keyStr));
        } else if (type == 2) {
            seq->addChild(KeyUpNode::make(key, keyStr));
        } else {
            warn("旧格式 KeyboardEvent 的 Type 既不是 1(Down) 也不是 2(Up)，已跳过");
            return nullptr;
        }

        if (seq->children.size() == 1) return std::move(seq->children[0]);
        return seq;
    }

    if (event.tag == "MouseMovementEvent") {
        int type = event.child_int("Type", 3);
        if (type != 3) {
            warn("旧格式 MouseMovementEvent 的 Type 不是 3，按移动事件继续解析");
        }

        int delay = event.child_int("Delay", 0);
        int x = event.child_int("X", 0);
        int y = event.child_int("Y", 0);

        auto seq = SequenceNode::make();
        if (delay > 0) seq->addChild(SleepNode::make(delay));
        auto moveNode = MouseMoveNode::make(CoordType::CoordAbsolute);
        moveNode->addChild(ExprNumberNode::make(x));
        moveNode->addChild(ExprNumberNode::make(y));
        seq->addChild(std::move(moveNode));

        if (seq->children.size() == 1) return std::move(seq->children[0]);
        return seq;
    }

    // 兼容场景：非事件标签直接忽略（如注释、未知字段）
    return nullptr;
}

// ── 鼠标移动序列 ─────────────────────────────────────────────────────────────
//
// 输入：<MouseMovement> 节点，含多个 <MouseMovementEvent>
// 输出：SequenceNode，内含交替的 SleepNode + MouseMoveNode
//
// 坐标处理：
//   雷蛇记录的是绝对坐标。相邻两点之间有 Delay（毫秒）。
//   第一个点的 Delay 通常为 0 或未填写，使用 MacroEvent.Delay 作为初始延迟。
//
// 结构：
//   SequenceNode
//   ├── SleepNode(initialDelay)      ← 来自 MacroEvent.Delay（若 > 0）
//   ├── MouseMoveNode(ABSOLUTE,x0,y0)← 第一个点（无前置延迟）
//   ├── SleepNode(delay1)            ← 第二个点的 Delay
//   ├── MouseMoveNode(ABSOLUTE,x1,y1)
//   ├── SleepNode(delay2)
//   ├── MouseMoveNode(ABSOLUTE,x2,y2)
//   ...

AIRNodePtr RazerParser::buildMouseMovement(const XmlNode& movement,
                                           int initialDelay)
{
    auto seq = SequenceNode::make();

    // 初始延迟
    if (initialDelay > 0)
        seq->addChild(SleepNode::make(initialDelay));

    auto movePoints = movement.children_of("MouseMovementEvent");

    if (movePoints.empty()) {
        warn("MouseMovement 中没有 MouseMovementEvent 子节点");
        return seq;
    }

    for (size_t i = 0; i < movePoints.size(); ++i) {
        const XmlNode* pt = movePoints[i];

        int x     = pt->child_int("X",     0);
        int y     = pt->child_int("Y",     0);
        int delay = pt->child_int("Delay", 0);
        // int moveType = pt->child_int("Type", 3); // 目前只见过 3

        // 第一个点没有前置延迟（已由 initialDelay 处理），
        // 后续点在移动前先 Sleep
        if (i > 0 && delay > 0)
            seq->addChild(SleepNode::make(delay));

        // MouseMoveNode（绝对坐标）
        auto moveNode = MouseMoveNode::make(CoordType::CoordAbsolute);
        moveNode->addChild(ExprNumberNode::make(x));
        moveNode->addChild(ExprNumberNode::make(y));
        seq->addChild(std::move(moveNode));
    }

    return seq;
}

// ── 键盘事件 ─────────────────────────────────────────────────────────────────

AIRNodePtr RazerParser::buildKeyboardEvent(const XmlNode& event, bool isDown)
{
    // 查找 <KeyboardEvent> → <Key>VK_xxx</Key>
    const XmlNode* kbEvent = event.child("KeyboardEvent");
    if (!kbEvent) {
        warn("键盘事件缺少 <KeyboardEvent> 子节点");
        return nullptr;
    }

    std::string keyStr = kbEvent->child_text("Key");
    if (keyStr.empty()) {
        warn("键盘事件 <Key> 为空");
        return nullptr;
    }

    AIRKey key = RazerKeyMap::RazerKeyToAIR(keyStr);

    if (isDown) {
        return KeyDownNode::make(key, keyStr);
    }
    else {
        return KeyUpNode::make(key, keyStr);
    }
}

// ── 鼠标按键事件 ─────────────────────────────────────────────────────────────

AIRNodePtr RazerParser::buildMouseButtonEvent(const XmlNode& event,
                                              bool isDown)
{
    // 查找 <MouseButtonEvent> → <Button>N</Button>
    const XmlNode* mbEvent = event.child("MouseButtonEvent");
    if (!mbEvent) {
        warn("鼠标按键事件缺少 <MouseButtonEvent> 子节点");
        return nullptr;
    }

    int buttonIndex = mbEvent->child_int("Button", 1);
    AIRKey key = RazerKeyMap::RazerMouseButtonToAIR(buttonIndex);

    if (isDown) return MouseDownNode::make(key);
    else        return MouseUpNode::make(key);
}

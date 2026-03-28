/*
 * RazerParser.h  —  雷蛇 XML 宏 → AIR 树解析器
 *
 * 解析策略：
 *   手写轻量 XML 解析器（不依赖第三方库）。
 *   雷蛇宏 XML 结构较规整，无需完整 XML 解析器。
 *
 * MacroEvent.Type 含义：
 *   1 = 键盘按下
 *   2 = 键盘松开
 *   3 = 鼠标移动序列（含子 MouseMovementEvent 列表）
 *   4 = 鼠标按键按下
 *   5 = 鼠标按键松开
 *   6 = 纯延迟
 *
 * MouseMovementEvent.Type 含义：
 *   3 = 绝对坐标移动（目前仅见此值）
 *
 * 转换说明（绝对坐标 → AHK）：
 *   雷蛇记录的是绝对屏幕坐标。
 *   解析时忠实保留为 ABSOLUTE MouseMoveNode。
 *   如用户需要相对坐标版本，可在主程序后处理层做坐标转换。
 *
 * 整体 AIR 结构：
 *   ProgramNode
 *   └── TriggerNode (MOUSE_BUTTON_DOWN, LEFT)   ← 宏由左键触发
 *       └── SequenceNode
 *           ├── SleepNode(initialDelay)          ← MacroEvent.Delay
 *           ├── MouseMoveNode(ABSOLUTE, x0, y0)  ← 第一个移动点
 *           ├── SleepNode(delay1)
 *           ├── MouseMoveNode(ABSOLUTE, x1, y1)
 *           ...（交替的 Sleep + MouseMove）
 *           ├── KeyDownNode / KeyUpNode           ← 键盘事件
 *           ├── MouseDownNode / MouseUpNode       ← 鼠标按键事件
 *           └── SleepNode                        ← 纯延迟事件
 */

#pragma once

#include "AIR.h"
#include <string>
#include <vector>

class RazerParser {
public:
    RazerParser(const std::string& xmlSource,
                std::vector<AIR::AIRDiagnostic>& diags);

    // 解析 XML，返回 ProgramNode 根节点
    AIR::AIRNodePtr parse();

private:
    // ── 轻量 XML 节点 ─────────────────────────────────────────────────────────
    struct XmlNode {
        std::string              tag;
        std::string              text;       // 直接文本内容
        std::vector<XmlNode>     children;
        // 按标签名查找第一个直接子节点
        const XmlNode* child(const std::string& tagName) const;
        // 按标签名查找所有直接子节点
        std::vector<const XmlNode*> children_of(const std::string& tagName) const;
        // 获取直接文本内容，去除首尾空白
        std::string text_trimmed() const;
        // 获取子节点的文本内容（常用辅助）
        std::string child_text(const std::string& tagName,
                               const std::string& defaultVal = {}) const;
        int child_int(const std::string& tagName, int defaultVal = 0) const;
    };

    // ── XML 解析 ──────────────────────────────────────────────────────────────
    XmlNode parseXml();                        // 解析整个 XML 文档
    XmlNode parseElement(size_t& pos);         // 解析一个 XML 元素
    void    skipWhitespace(size_t& pos) const;
    void    skipComment(size_t& pos) const;    // 跳过 <!-- ... -->
    void    skipDeclaration(size_t& pos) const;// 跳过 <?xml ... ?>
    std::string parseTagName(size_t& pos) const;
    std::string parseText(size_t& pos, const std::string& endTag) const;
    void    skipAttributes(size_t& pos) const;

    // ── AIR 构建 ──────────────────────────────────────────────────────────────
    AIR::AIRNodePtr buildProgram(const XmlNode& macroRoot);
    AIR::AIRNodePtr buildSequence(const XmlNode& macroEvents);
    AIR::AIRNodePtr buildMacroEvent(const XmlNode& event);
    AIR::AIRNodePtr buildMouseMovement(const XmlNode& movement,
                                       int            initialDelay);
    AIR::AIRNodePtr buildKeyboardEvent(const XmlNode& event, bool isDown);
    AIR::AIRNodePtr buildMouseButtonEvent(const XmlNode& event, bool isDown);

    // ── 工具 ──────────────────────────────────────────────────────────────────
    void warn(const std::string& msg, const std::string& suggestion = {});
    void error(const std::string& msg);

    std::string                      m_source;
    std::vector<AIR::AIRDiagnostic>& m_diags;
};

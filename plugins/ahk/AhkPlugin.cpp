/*
 * AhkPlugin.cpp  —  AHK 宏插件主类 + DLL 入口
 */

#include "AhkPlugin.h"
#include "AhkParser.h"
#include "AhkGenerator.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
#endif

// ============================================================================
//  AhkPlugin：IScriptPlugin 实现
// ============================================================================

class AhkPlugin : public AIR::IScriptPlugin {
public:
    const char* GetFormatName()    const override { return "AHKMacro"; }
    const char* GetFileExtension() const override { return ".ahk"; }
    const char* GetPluginVersion() const override { return "1.0.0"; }
    const char* GetDemoCode() const override {
        return R"(
; 这是一个简单的 AHK 脚本示例，演示了热键和鼠标操作
~LButton::
{
    while (GetKeyState("LButton", "P")) {
        MouseMove 0, 3, 0, "R"
        Sleep Random(20, 30)
    }
}
)";
    }

    AIR::AIRNodePtr Parse(
        const std::string& sourceText,
        std::vector<AIR::AIRDiagnostic>& outDiags) override
    {
        try {
            AhkParser parser(sourceText, outDiags);
            return parser.parse();
        }
        catch (const std::exception& e) {
            outDiags.push_back({ AIR::DiagLevel::Error, 0,
                std::string("Parse 异常: ") + e.what(), {} });
            return nullptr;
        }
        catch (...) {
            outDiags.push_back({ AIR::DiagLevel::Error, 0,
                "Parse 发生未知异常", {} });
            return nullptr;
        }
    }

    std::string Generate(
        const AIR::AIRNode& root,
        std::vector<AIR::AIRDiagnostic>& outDiags) override
    {
        try {
            AhkGenerator gen(outDiags);
            const_cast<AIR::AIRNode&>(root).accept(gen);
            return gen.result();
        }
        catch (const std::exception& e) {
            outDiags.push_back({ AIR::DiagLevel::Error, 0,
                std::string("Generate 异常: ") + e.what(), {} });
            return {};
        }
        catch (...) {
            outDiags.push_back({ AIR::DiagLevel::Error, 0,
                "Generate 发生未知异常", {} });
            return {};
        }
    }
};

// ============================================================================
//  导出工厂函数
// ============================================================================

extern "C" {

    AHKPLUGIN_API AIR::IScriptPlugin* CreateAhkPlugin() {  return new AhkPlugin();    }
    AHKPLUGIN_API void DestroyAhkPlugin(AIR::IScriptPlugin* plugin) {delete plugin;   }
    AHKPLUGIN_API AIR::IScriptPlugin* CreatePlugin() { return new AhkPlugin(); }
    AHKPLUGIN_API void DestroyPlugin(AIR::IScriptPlugin* p) { delete p; }


} // extern "C"

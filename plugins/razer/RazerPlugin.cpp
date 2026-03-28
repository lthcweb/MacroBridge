/*
 * RazerPlugin.cpp  —  雷蛇宏插件主类 + DLL 入口
 */

#include "RazerPlugin.h"
#include "RazerParser.h"
#include "RazerGenerator.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
#endif

// ============================================================================
//  RazerPlugin：IScriptPlugin 实现
// ============================================================================

class RazerPlugin : public AIR::IScriptPlugin {
public:
    const char* GetFormatName()    const override { return "RazerMacro"; }
    const char* GetFileExtension() const override { return ".xml"; }
    const char* GetPluginVersion() const override { return "1.0.0"; }
    const char* GetDemoCode() const override {
        return R"(
-- Razer 宏示例
<?xml version="1.0" encoding="utf-8"?>
<RazerMacro>
  <Name>MacroBridge_Recoil_Assist</Name>
  <MacroEvents>
    <MouseButtonEvent>
      <Type>1</Type> <Button>1</Button> </MouseButtonEvent>
    
    <MouseMovementEvent>
      <Type>3</Type> <X>0</X>
      <Y>3</Y>
    </MouseMovementEvent>

    <DelayEvent>
      <Delay>25</Delay>
    </DelayEvent>

    <MouseButtonEvent>
      <Type>2</Type> <Button>1</Button>
    </MouseButtonEvent>
  </MacroEvents>
</RazerMacro>
)";
    }

    AIR::AIRNodePtr Parse(
        const std::string&               sourceText,
        std::vector<AIR::AIRDiagnostic>& outDiags) override
    {
        try {
            RazerParser parser(sourceText, outDiags);
            return parser.parse();
        } catch (const std::exception& e) {
            outDiags.push_back({ AIR::DiagLevel::Error, 0,
                std::string("Parse 异常: ") + e.what(), {} });
            return nullptr;
        } catch (...) {
            outDiags.push_back({ AIR::DiagLevel::Error, 0,
                "Parse 发生未知异常", {} });
            return nullptr;
        }
    }

    std::string Generate(
        const AIR::AIRNode&              root,
        std::vector<AIR::AIRDiagnostic>& outDiags) override
    {
        try {
            RazerGenerator gen(outDiags);
            const_cast<AIR::AIRNode&>(root).accept(gen);
            return gen.result();
        } catch (const std::exception& e) {
            outDiags.push_back({ AIR::DiagLevel::Error, 0,
                std::string("Generate 异常: ") + e.what(), {} });
            return {};
        } catch (...) {
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

    RAZERPLUGIN_API AIR::IScriptPlugin* CreateRazerPlugin() {    return new RazerPlugin();}
    RAZERPLUGIN_API void DestroyRazerPlugin(AIR::IScriptPlugin* plugin) {   delete plugin;}
    RAZERPLUGIN_API AIR::IScriptPlugin* CreatePlugin() { return new RazerPlugin(); }
    RAZERPLUGIN_API void DestroyPlugin(AIR::IScriptPlugin* p) { delete p; }

} // extern "C"

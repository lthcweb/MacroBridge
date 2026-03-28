/*
 * LogitechPlugin.cpp  —  罗技 Lua 宏插件 DLL 入口
 */
#include "LogitechPlugin.h"
#include "LogitechParser.h"
#include "LogitechGenerator.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
#endif

class LogitechPluginImpl : public AIR::IScriptPlugin {
public:
    const char* GetFormatName()    const override { return "LogitechMacro"; }
    const char* GetFileExtension() const override { return ".lua"; }
    const char* GetPluginVersion() const override { return "1.0.0"; }
    const char* GetDemoCode() const override {
        return R"(
-- Logitech G-Series Lua 示例
-- 逻辑：按下左键(1)时，循环向下移动鼠标
function OnEvent(event, arg)
    -- 监听鼠标左键按下事件
    if event == "MOUSE_BUTTON_PRESSED" and arg == 1 then
        repeat
            -- 相对移动：x=0, y=3
            MoveMouseRelative(0, 3)
            
            -- 随机延迟 20ms 到 30ms
            Sleep(math.random(20, 30))
            
            -- 循环直到左键松开
        until not IsMouseButtonPressed(1)
    end
end
)";
    }

    AIR::AIRNodePtr Parse(const std::string& src,
                          std::vector<AIR::AIRDiagnostic>& diags) override {
        try {
            LogitechParser p(src, diags);
            return p.parse();
        } catch (const std::exception& e) {
            diags.push_back({AIR::DiagLevel::Error,0,std::string("Parse异常:")+e.what(),{}});
            return nullptr;
        } catch (...) {
            diags.push_back({AIR::DiagLevel::Error,0,"Parse未知异常",{}});
            return nullptr;
        }
    }

    std::string Generate(const AIR::AIRNode& root,
                         std::vector<AIR::AIRDiagnostic>& diags) override {
        try {
            LogitechGenerator g(diags);
            const_cast<AIR::AIRNode&>(root).accept(g);
            return g.result();
        } catch (const std::exception& e) {
            diags.push_back({AIR::DiagLevel::Error,0,std::string("Generate异常:")+e.what(),{}});
            return {};
        } catch (...) {
            diags.push_back({AIR::DiagLevel::Error,0,"Generate未知异常",{}});
            return {};
        }
    }
};

extern "C" {
LOGITECHPLUGIN_API AIR::IScriptPlugin* CreateLogitechPlugin() { return new LogitechPluginImpl(); }
LOGITECHPLUGIN_API void DestroyLogitechPlugin(AIR::IScriptPlugin* p) { delete p; }
LOGITECHPLUGIN_API AIR::IScriptPlugin* CreatePlugin()      { return new LogitechPluginImpl(); }
LOGITECHPLUGIN_API void DestroyPlugin(AIR::IScriptPlugin* p)    { delete p; }
}

#include "XMousePlugin.h"
#include "XMouseParser.h"
#include "XMouseGenerator.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
#endif

class XMousePlugin : public AIR::IScriptPlugin {
public:
    const char* GetFormatName() const override { return "XMouseProfile"; }
    const char* GetFileExtension() const override { return ".xmbcs"; }
    const char* GetPluginVersion() const override { return "1.0.0"; }
    const char* GetDemoCode() const override {
        return R"(<?xml version="1.0" encoding="utf-8"?>
<XMouseProfiles>
  <Profile Name="MacroBridge_Demo">
    <Layer Index="1">
      <Button Name="XButton1" Action="SimulatedKeys" Keys="{CTRL}C{WAITMS:40}{CTRL}V"/>
    </Layer>
  </Profile>
</XMouseProfiles>
)";
    }

    AIR::AIRNodePtr Parse(const std::string& sourceText,
                          std::vector<AIR::AIRDiagnostic>& outDiags) override {
        XMouseParser parser(sourceText, outDiags);
        return parser.parse();
    }

    std::string Generate(const AIR::AIRNode& root,
                         std::vector<AIR::AIRDiagnostic>& outDiags) override {
        XMouseGenerator gen(outDiags);
        return gen.generate(root);
    }
};

extern "C" {
XMOUSEPLUGIN_API AIR::IScriptPlugin* CreateXMousePlugin() { return new XMousePlugin(); }
XMOUSEPLUGIN_API void DestroyXMousePlugin(AIR::IScriptPlugin* plugin) { delete plugin; }
XMOUSEPLUGIN_API AIR::IScriptPlugin* CreatePlugin() { return new XMousePlugin(); }
XMOUSEPLUGIN_API void DestroyPlugin(AIR::IScriptPlugin* plugin) { delete plugin; }
}

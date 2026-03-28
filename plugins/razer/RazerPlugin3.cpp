#include "RazerPlugin3.h"
#include "RazerParser.h"
#include "RazerGenerator.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
#endif

class RazerPlugin3 : public AIR::IScriptPlugin {
public:
    const char* GetFormatName() const override { return "RazerPlugin3"; }
    const char* GetFileExtension() const override { return ".xml"; }
    const char* GetPluginVersion() const override { return "3.0.0"; }
    const char* GetDemoCode() const override {
        return R"(<?xml version="1.0" encoding="utf-8"?>
<RazerMacro>
  <Name>Synapse3_Demo</Name>
  <MacroEvents>
    <MouseButtonEvent><Type>1</Type><Button>1</Button></MouseButtonEvent>
    <MouseMovementEvent><Type>3</Type><X>0</X><Y>3</Y><Delay>25</Delay></MouseMovementEvent>
    <DelayEvent><Delay>30</Delay></DelayEvent>
    <MouseButtonEvent><Type>2</Type><Button>1</Button></MouseButtonEvent>
  </MacroEvents>
</RazerMacro>
)";
    }

    AIR::AIRNodePtr Parse(const std::string& sourceText,
                          std::vector<AIR::AIRDiagnostic>& outDiags) override {
        RazerParser parser(sourceText, outDiags, RazerSynapseVersion::Synapse3);
        return parser.parse();
    }

    std::string Generate(const AIR::AIRNode& root,
                         std::vector<AIR::AIRDiagnostic>& outDiags) override {
        RazerGenerator gen(outDiags, RazerSynapseVersion::Synapse3);
        const_cast<AIR::AIRNode&>(root).accept(gen);
        return gen.result();
    }
};

extern "C" {
RAZERPLUGIN3_API AIR::IScriptPlugin* CreateRazerPlugin3() { return new RazerPlugin3(); }
RAZERPLUGIN3_API void DestroyRazerPlugin3(AIR::IScriptPlugin* plugin) { delete plugin; }
RAZERPLUGIN3_API AIR::IScriptPlugin* CreatePlugin() { return new RazerPlugin3(); }
RAZERPLUGIN3_API void DestroyPlugin(AIR::IScriptPlugin* plugin) { delete plugin; }
}

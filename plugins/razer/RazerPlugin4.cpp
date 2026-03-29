#include "RazerPlugin4.h"
#include "RazerParser.h"
#include "RazerGenerator.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
#endif

class RazerPlugin4 : public AIR::IScriptPlugin {
public:
    const char* GetFormatName() const override { return "Razer4Plugin"; }
    const char* GetFileExtension() const override { return ".xml"; }
    const char* GetPluginVersion() const override { return "4.0.0"; }
    const char* GetDemoCode() const override {
        return R"(<?xml version="1.0" encoding="utf-8"?>
<Macro xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema">
  <Name>Synapse4_Demo</Name>
  <Guid>00000000-0000-0000-0000-000000000000</Guid>
  <MacroEvents>
    <MacroEvent><Type>4</Type><Delay>0</Delay><MouseButtonEvent><Button>1</Button></MouseButtonEvent></MacroEvent>
    <MacroEvent><Type>3</Type><Delay>0</Delay><MouseMovement><MouseMovementEvent><Type>3</Type><X>0</X><Y>3</Y><Delay>25</Delay></MouseMovementEvent></MouseMovement></MacroEvent>
    <MacroEvent><Type>5</Type><Delay>30</Delay><MouseButtonEvent><Button>1</Button></MouseButtonEvent></MacroEvent>
  </MacroEvents>
</Macro>
)";
    }

    AIR::AIRNodePtr Parse(const std::string& sourceText,
                          std::vector<AIR::AIRDiagnostic>& outDiags) override {
        RazerParser parser(sourceText, outDiags, RazerSynapseVersion::Synapse4);
        return parser.parse();
    }

    std::string Generate(const AIR::AIRNode& root,
                         std::vector<AIR::AIRDiagnostic>& outDiags) override {
        RazerGenerator gen(outDiags, RazerSynapseVersion::Synapse4);
        const_cast<AIR::AIRNode&>(root).accept(gen);
        return gen.result();
    }
};

extern "C" {
RAZER4PLUGIN_API AIR::IScriptPlugin* CreateRazer4Plugin() { return new RazerPlugin4(); }
RAZER4PLUGIN_API void DestroyRazer4Plugin(AIR::IScriptPlugin* plugin) { delete plugin; }
RAZER4PLUGIN_API AIR::IScriptPlugin* CreatePlugin() { return new RazerPlugin4(); }
RAZER4PLUGIN_API void DestroyPlugin(AIR::IScriptPlugin* plugin) { delete plugin; }
}

#pragma once

#include "AIR.h"

#ifdef _WIN32
#  ifdef RAZER4PLUGIN_EXPORTS
#    define RAZER4PLUGIN_API __declspec(dllexport)
#  else
#    define RAZER4PLUGIN_API __declspec(dllimport)
#  endif
#else
#  define RAZER4PLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {
    RAZER4PLUGIN_API AIR::IScriptPlugin* CreateRazer4Plugin();
    RAZER4PLUGIN_API void DestroyRazer4Plugin(AIR::IScriptPlugin* plugin);
    RAZER4PLUGIN_API AIR::IScriptPlugin* CreatePlugin();
    RAZER4PLUGIN_API void DestroyPlugin(AIR::IScriptPlugin* plugin);
}

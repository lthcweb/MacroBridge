#pragma once

#include "AIR.h"

#ifdef _WIN32
#  ifdef RAZER3PLUGIN_EXPORTS
#    define RAZER3PLUGIN_API __declspec(dllexport)
#  else
#    define RAZER3PLUGIN_API __declspec(dllimport)
#  endif
#else
#  define RAZER3PLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {
    RAZER3PLUGIN_API AIR::IScriptPlugin* CreateRazer3Plugin();
    RAZER3PLUGIN_API void DestroyRazer3Plugin(AIR::IScriptPlugin* plugin);
    RAZER3PLUGIN_API AIR::IScriptPlugin* CreatePlugin();
    RAZER3PLUGIN_API void DestroyPlugin(AIR::IScriptPlugin* plugin);
}

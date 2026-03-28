#pragma once

#include "AIR.h"

#ifdef _WIN32
#  ifdef RAZERPLUGIN4_EXPORTS
#    define RAZERPLUGIN4_API __declspec(dllexport)
#  else
#    define RAZERPLUGIN4_API __declspec(dllimport)
#  endif
#else
#  define RAZERPLUGIN4_API __attribute__((visibility("default")))
#endif

extern "C" {
    RAZERPLUGIN4_API AIR::IScriptPlugin* CreateRazerPlugin4();
    RAZERPLUGIN4_API void DestroyRazerPlugin4(AIR::IScriptPlugin* plugin);
    RAZERPLUGIN4_API AIR::IScriptPlugin* CreatePlugin();
    RAZERPLUGIN4_API void DestroyPlugin(AIR::IScriptPlugin* plugin);
}

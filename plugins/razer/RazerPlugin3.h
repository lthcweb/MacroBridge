#pragma once

#include "AIR.h"

#ifdef _WIN32
#  ifdef RAZERPLUGIN3_EXPORTS
#    define RAZERPLUGIN3_API __declspec(dllexport)
#  else
#    define RAZERPLUGIN3_API __declspec(dllimport)
#  endif
#else
#  define RAZERPLUGIN3_API __attribute__((visibility("default")))
#endif

extern "C" {
    RAZERPLUGIN3_API AIR::IScriptPlugin* CreateRazerPlugin3();
    RAZERPLUGIN3_API void DestroyRazerPlugin3(AIR::IScriptPlugin* plugin);
    RAZERPLUGIN3_API AIR::IScriptPlugin* CreatePlugin();
    RAZERPLUGIN3_API void DestroyPlugin(AIR::IScriptPlugin* plugin);
}

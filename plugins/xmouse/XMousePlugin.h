#pragma once

#include "AIR.h"

#ifdef _WIN32
#  ifdef XMOUSEPLUGIN_EXPORTS
#    define XMOUSEPLUGIN_API __declspec(dllexport)
#  else
#    define XMOUSEPLUGIN_API __declspec(dllimport)
#  endif
#else
#  define XMOUSEPLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {
    XMOUSEPLUGIN_API AIR::IScriptPlugin* CreateXMousePlugin();
    XMOUSEPLUGIN_API void DestroyXMousePlugin(AIR::IScriptPlugin* plugin);
    XMOUSEPLUGIN_API AIR::IScriptPlugin* CreatePlugin();
    XMOUSEPLUGIN_API void DestroyPlugin(AIR::IScriptPlugin* plugin);
}

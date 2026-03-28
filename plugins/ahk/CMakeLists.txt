# plugins/ahk/CMakeLists.txt
#
# AHK 插件：解析和生成 AutoHotkey v2 格式脚本。

add_plugin(
    NAME    AhkPlugin
    SOURCES
        AhkPlugin.cpp
        AhkParser.cpp
        AhkGenerator.cpp
        AhkKeyMap.cpp
)

# plugins/razer/CMakeLists.txt
#
# 雷蛇插件：解析和生成雷蛇 Synapse XML 格式宏。

add_plugin(
    NAME    RazerPlugin
    SOURCES
        RazerPlugin.cpp
        RazerParser.cpp
        RazerGenerator.cpp
        RazerKeyMap.cpp
)

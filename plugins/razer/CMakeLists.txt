# plugins/razer/CMakeLists.txt
#
# 雷蛇插件：解析和生成雷蛇 Synapse XML 格式宏。

add_plugin(
    NAME    RazerPlugin3
    SOURCES
        RazerPlugin3.cpp
        RazerParser.cpp
        RazerGenerator.cpp
        RazerKeyMap.cpp
)

add_plugin(
    NAME    RazerPlugin4
    SOURCES
        RazerPlugin4.cpp
        RazerParser.cpp
        RazerGenerator.cpp
        RazerKeyMap.cpp
)

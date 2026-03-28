# cmake/PluginHelpers.cmake
#
# 提供 add_plugin() 辅助宏，统一处理所有插件 DLL 的公共设置。
#
# 用法：
#   add_plugin(
#       NAME    AhkPlugin                  # 目标名（同时是 DLL 文件名）
#       SOURCES                            # 源文件列表
#           AhkPlugin.cpp
#           AhkParser.cpp
#           AhkGenerator.cpp
#           AhkKeyMap.cpp
#   )
#
# 该宏自动完成：
#   - 创建 SHARED 库目标
#   - 引入 shared/ 公共头文件目录
#   - 定义 <NAME>_EXPORTS 宏（控制 dllexport/dllimport）
#   - 去掉 "lib" 前缀（Windows DLL 不需要）
#   - 输出目录与主程序保持一致（已由根 CMakeLists 设置全局变量）
#
# 新增插件只需在 plugins/<name>/CMakeLists.txt 里调用此宏，
# 无需重复写编译选项。

macro(add_plugin)
    # 解析参数
    cmake_parse_arguments(
        PLUGIN          # 前缀
        ""              # 布尔选项（无）
        "NAME"          # 单值选项
        "SOURCES;INCLUDES"  # 多值选项
        ${ARGN}
    )

    if(NOT PLUGIN_NAME)
        message(FATAL_ERROR "add_plugin: 必须提供 NAME 参数")
    endif()
    if(NOT PLUGIN_SOURCES)
        message(FATAL_ERROR "add_plugin: 必须提供 SOURCES 参数")
    endif()

    # 创建 DLL 目标
    add_library(${PLUGIN_NAME} SHARED ${PLUGIN_SOURCES})

    # 引入公共头文件（AIR.h 等）
    target_include_directories(${PLUGIN_NAME}
        PRIVATE
            ${MACRO_SHARED_INCLUDE_DIR}   # shared/AIR.h
            ${CMAKE_CURRENT_SOURCE_DIR}   # 插件自身目录
            ${PLUGIN_INCLUDES}            # 调用方额外指定的 include 路径
    )

    # 定义导出宏，例如 AHKPLUGIN_EXPORTS
    string(TOUPPER ${PLUGIN_NAME} _PLUGIN_NAME_UPPER)
    target_compile_definitions(${PLUGIN_NAME}
        PRIVATE ${_PLUGIN_NAME_UPPER}_EXPORTS
    )

    # 去掉 "lib" 前缀（在 Linux/macOS 上生成 AhkPlugin.so 而非 libAhkPlugin.so）
    set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")

    message(STATUS "Plugin registered: ${PLUGIN_NAME}")
endmacro()

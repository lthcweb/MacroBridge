# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.20)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(MacroBridge
    VERSION     0.1.0
    DESCRIPTION "游戏宏跨平台转换工具"
    LANGUAGES   CXX
)

# ============================================================================
#  全局 C++ 标准
# ============================================================================
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ============================================================================
#  统一输出目录
#  所有 .exe 和 .dll 都输出到同一个 bin/<Config>/ 目录，
#  这样主程序能直接在同目录找到插件 DLL，无需配置 PATH。
# ============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# 多配置生成器（VS / Ninja Multi-Config）自动按 <Config> 子目录分开
foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${cfg} CFG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} "${CMAKE_BINARY_DIR}/bin/${cfg}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG} "${CMAKE_BINARY_DIR}/bin/${cfg}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFG} "${CMAKE_BINARY_DIR}/lib/${cfg}")
endforeach()

# ============================================================================
#  公共头文件目录（shared/）
#  所有子目标通过 MACRO_SHARED_INCLUDE_DIR 引用 AIR.h
# ============================================================================
set(MACRO_SHARED_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shared")

# ============================================================================
#  载入公用 CMake 辅助宏
# ============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PluginHelpers)

# ============================================================================
#  MSVC 全局编译规范
#  - /MD  : 动态链接 CRT（插件与主程序共享同一堆，避免跨堆崩溃）
#  - /utf-8 : 源文件 UTF-8 编码
#  - Unicode : Win32 API 使用宽字符版本
# ============================================================================
if(MSVC)
    # 将所有 /MT、/MTd 替换为 /MD、/MDd
    foreach(flag_var
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        string(REGEX REPLACE "/MT([d]?)" "/MD\\1" ${flag_var} "${${flag_var}}")
    endforeach()

    add_compile_definitions(UNICODE _UNICODE)
    add_compile_options(
        /W4         # 警告等级 4
        /utf-8      # 源文件 UTF-8
        /MP         # 并行编译
    )
    # 关闭几个常见的 "噪音" 警告
    add_compile_options(
        /wd4100     # 未使用的函数参数
        /wd4505     # 未引用的本地函数
    )
endif()

# ============================================================================
#  子目录
# ============================================================================

# 主程序（exe）
add_subdirectory(app)

# 所有插件（DLL）
add_subdirectory(plugins)
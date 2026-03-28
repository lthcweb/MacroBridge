# CMakeList.txt: MacroBridge 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

# app/CMakeLists.txt
#
# 主程序（MacroBridge.exe）
set(PROJECT_NAME MacroBridge)
PROJECT(${PROJECT_NAME})

# 1. 确保在 project() 之后立即设置运行时库
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(COMMON_DIR "${CMAKE_SOURCE_DIR}/Common")
set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/ThirdParty")

# ── 收集所有源文件 ──────────────────────────────────────────────────────────
set(APP_HEADERS
    ${COMMON_DIR}/UIBinding/RuleBuilder.h
    ${COMMON_DIR}/UIBinding/UIBindingRules.h

    "src/Manager/PluginManager.h"

    src/Manager/AppManager.h
    src/Manager/ResourceManager.h

    src/UI/Main/Windows/MainWindow.h
    src/UI/Main/Views/MainWindowUI.h
    src/UI/Main/Core/UIPageBase.h
    
)

set(APP_SOURCES
    ${COMMON_DIR}/UIBinding/RuleBuilder.cpp
    ${COMMON_DIR}/UIBinding/UIBindingRules.cpp

    src/main.cpp
    "src/Manager/PluginManager.cpp"

    src/Manager/AppManager.cpp
    src/Manager/ResourceManager.cpp

    src/UI/Main/Windows/MainWindow.cpp
    src/UI/Main/Views/MainWindowUI.cpp
    src/UI/Main/Core/UIPageBase.cpp
)

add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES} ${APP_HEADERS})
# WIN32 表示不弹出控制台窗口（GUI 程序）；
# 调试阶段若想看控制台输出，可临时改为：
#   add_executable(MacroConverter ${APP_SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${MACRO_SHARED_INCLUDE_DIR}   # shared/AIR.h
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ── 链接系统库 ───────────────────────────────────────────────────────────────
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        # Windows 系统库
        user32
        gdi32
        shell32
        ole32
        oleaut32
        comctl32    # 公共控件（DuiLib 需要）
        imm32       # 输入法
        # DuiLib（待集成后取消注释，并设置 DUILIB_DIR）
        # ${DUILIB_DIR}/lib/DuiLib_d.lib   # Debug
        # ${DUILIB_DIR}/lib/DuiLib.lib     # Release
    )
endif()

# ── 编译后自动将插件 DLL 复制到 exe 同目录 ──────────────────────────────────
# 这样 F5 直接运行时就能找到 DLL，无需手动复制
# （也可以不复制，因为输出目录已经统一了，取决于个人喜好）
#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E echo
#        "构建完成：$<TARGET_FILE:MacroConverter>"
#    VERBATIM
#)

# ── 设置调试工作目录（VS 调试时自动切换到 exe 所在目录）──────────────────────
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)


# THIRDPARTY_DIR  
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}")

target_compile_definitions(${PROJECT_NAME} PRIVATE UILIB_STATIC)
set(DUILIB_ROOT "${CMAKE_SOURCE_DIR}/ThirdParty/DuiLib")
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${DUILIB_ROOT}/include")
target_link_directories(${PROJECT_NAME} PRIVATE "${DUILIB_ROOT}/lib")
target_link_libraries(${PROJECT_NAME} PRIVATE 
    debug "${DUILIB_ROOT}/lib/DuiLib_d.lib"
    optimized "${DUILIB_ROOT}/lib/DuiLib.lib"
)

target_compile_options(${PROJECT_NAME}  PRIVATE /wd4702) # 无法访问的代码


# ── 同步皮肤资源 ─────────────────────────────────────────────────────────────
# 1. 设置路径（利用 CMAKE_BINARY_DIR 对应 JSON 中的路径）
set(SKIN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/skin")
# 这里直接硬编码到你观察到的 bin/Debug 目录
set(SKIN_TARGET_DIR "${CMAKE_BINARY_DIR}/bin/Debug/skin")

# 2. 获取所有源文件列表，用于精确对比时间戳
file(GLOB_RECURSE SKIN_FILES "${SKIN_SOURCE_DIR}/*")

# 3. 定义一个自定义目标 (sync_skins)
# 关键字 ALL 确保它每次构建都会被检查
add_custom_target(sync_skins ALL
    # 使用 copy_directory：它本身就会检查文件，只有源文件比目标文件新时才复制
    COMMAND ${CMAKE_COMMAND} -E echo ">>>> [Skin Check] Checking for changes in ${SKIN_SOURCE_DIR}..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SKIN_SOURCE_DIR}" "${SKIN_TARGET_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo ">>>> [Skin Check] Sync completed."
    
    # 这一行很关键：将文件列表作为 SOURCES，
    # 这样在一些 IDE 里修改这些文件后，点击生成会更灵敏地触发检查
    SOURCES ${SKIN_FILES}
    
    COMMENT "Checking skin resources every build..."
    VERBATIM
)

# 4. 让主程序依赖这个同步目标
# 这样即使 C++ 没变，只要点击生成 MacroBridge，就会先运行 sync_skins
add_dependencies(MacroBridge sync_skins)




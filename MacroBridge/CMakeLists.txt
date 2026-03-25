# CMakeList.txt: MacroBridge 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME MacroBridge)

PROJECT(${PROJECT_NAME})

# 自动搜集当前目录下的所有源代码
file(GLOB_RECURSE SRC_LIST 
    "src/*.cpp" 
    "src/*.h"
    "src/*.hpp"  
)

# 将源代码添加到此项目的可执行文件。
add_executable (${PROJECT_NAME} ${SRC_LIST})

# GUI库 DuiLib 
set(DUILIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/DuiLib")
target_include_directories(${PROJECT_NAME} PRIVATE "${DUILIB_ROOT}/include")
target_link_directories(${PROJECT_NAME} PRIVATE "${DUILIB_ROOT}/lib")
target_link_libraries(${PROJECT_NAME} PRIVATE 
    debug "${DUILIB_ROOT}/lib/DuiLib_d.lib"
    optimized "${DUILIB_ROOT}/lib/DuiLib.lib"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()


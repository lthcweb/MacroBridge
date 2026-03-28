# plugins/CMakeLists.txt
#
# 插件目录的总调度文件。
# 每个子目录对应一个插件 DLL。
#
# 新增插件步骤：
#   1. 在 plugins/ 下建立新子目录，如 plugins/logitech/
#   2. 在该目录下编写代码和 CMakeLists.txt（调用 add_plugin 宏）
#   3. 在本文件末尾添加一行 add_subdirectory(logitech)
#   4. 重新运行 cmake，无需修改其他任何文件

add_subdirectory(ahk)
add_subdirectory(razer)

# 待实现：
 add_subdirectory(logitech)
# add_subdirectory(bloody)
# add_subdirectory(steelseries)

# MacroBridge  —  项目结构与构建说明

## 完整目录树

```
MacroBridge/
│
├── CMakeLists.txt                  ← 根 CMake（全局设置、子目录调度）
│
├── cmake/
│   └── PluginHelpers.cmake         ← add_plugin() 辅助宏
│
├── shared/                         ← 所有目标共用，纯头文件，无需编译
│   ├── AIR.h                       ← 中间表示层（核心接口）
│   └── PluginExportAlias.h         ← CreatePlugin/DestroyPlugin 通用别名
│
├── app/                            ← 主程序（exe）
│   ├── CMakeLists.txt
│   ├── main.cpp                    ← 程序入口（WinMain）
│   ├── PluginManager.h/.cpp        ← 插件加载/卸载管理
│   └── ui/                         ← DuiLib UI（待实现）
│       ├── MainWindow.h/.cpp
│       └── ConvertPanel.h/.cpp
│
└── plugins/                        ← 所有插件（每个子目录 = 一个 DLL）
    ├── CMakeLists.txt              ← 插件总调度（add_subdirectory）
    │
    ├── ahk/                        ← AHK v2 插件
    │   ├── CMakeLists.txt          ← 3 行：调用 add_plugin()
    │   ├── AhkPlugin.h/.cpp        ← IScriptPlugin 实现 + DLL 入口
    │   ├── AhkParser.h/.cpp        ← AHK 文本 → AIR 树
    │   ├── AhkGenerator.h/.cpp     ← AIR 树 → AHK 文本
    │   └── AhkKeyMap.h/.cpp        ← AIRKey ↔ AHK 键名映射
    │
    ├── razer/                      ← 雷蛇 Synapse XML 插件
    │   ├── CMakeLists.txt
    │   ├── RazerPlugin.h/.cpp
    │   ├── RazerParser.h/.cpp
    │   ├── RazerGenerator.h/.cpp
    │   └── RazerKeyMap.h/.cpp
    │
    └── logitech/                   ← 罗技 G HUB Lua 插件（待实现）
        ├── CMakeLists.txt
        └── ...
```

---

## 编译输出

```
build/
├── bin/
│   ├── Debug/
│   │   ├── MacroBridge.exe         ← 主程序
│   │   ├── AhkPlugin.dll           ← AHK 插件
│   │   └── RazerPlugin.dll         ← 雷蛇插件
│   └── Release/
│       ├── MacroBridge.exe
│       ├── AhkPlugin.dll
│       └── RazerPlugin.dll
└── lib/
    ├── Debug/
    └── Release/
```

**exe 和所有 DLL 在同一目录**，PluginManager 扫描 `*Plugin.dll` 即可加载全部插件，无需配置 PATH 或注册表。

---

## 构建步骤

```bash
# 1. 克隆或解压项目
cd MacroBridge

# 2. 配置（Visual Studio 2022）
cmake -B build -G "Visual Studio 17 2022" -A x64

# 3. 编译 Debug
cmake --build build --config Debug

# 4. 编译 Release
cmake --build build --config Release

# 5. 运行
build/bin/Debug/MacroBridge.exe
```

也可以直接用 VS 打开 `build/MacroBridge.sln`。

---

## 新增插件步骤（3 步）

**第 1 步**：建立目录

```
plugins/logitech/
├── CMakeLists.txt
├── LogitechPlugin.h/.cpp
├── LogitechParser.h/.cpp
├── LogitechGenerator.h/.cpp
└── LogitechKeyMap.h/.cpp
```

**第 2 步**：写 CMakeLists.txt（3 行）

```cmake
add_plugin(
    NAME    LogitechPlugin
    SOURCES LogitechPlugin.cpp LogitechParser.cpp
            LogitechGenerator.cpp LogitechKeyMap.cpp
)
```

**第 3 步**：在 `plugins/CMakeLists.txt` 末尾加一行

```cmake
add_subdirectory(logitech)
```

重新运行 `cmake --build build`，新插件自动加入编译，主程序自动发现并加载，**无需修改主程序任何代码**。

---

## 关键设计说明

### shared/ 是纯头文件，不编译为库

`AIR.h` 只有类型定义和 inline 方法，没有需要单独编译的 `.cpp`。
所有目标通过 `target_include_directories` 引用 `shared/` 即可，
无需链接任何静态库。

### /MD 强制动态 CRT

根 `CMakeLists.txt` 全局将 `/MT` 替换为 `/MD`，
确保主程序和所有插件共享同一个 CRT 堆，
避免跨 DLL 边界 `new`/`delete` 导致的堆损坏崩溃。

### 通用导出别名

每个插件除了导出 `CreateXxxPlugin()` 之外，
还通过 `shared/PluginExportAlias.h` 额外导出：

```c
AIR::IScriptPlugin* CreatePlugin();   // PluginManager 使用此名
void                DestroyPlugin(AIR::IScriptPlugin*);
```

这样 PluginManager 只需 `GetProcAddress(hMod, "CreatePlugin")`，
不需要知道每个插件的具体函数名。

### VS_DEBUGGER_WORKING_DIRECTORY

`app/CMakeLists.txt` 设置了调试工作目录为 exe 所在目录，
在 Visual Studio 按 F5 调试时，当前目录就是 `bin/Debug/`，
PluginManager 扫描当前目录能直接找到 DLL。

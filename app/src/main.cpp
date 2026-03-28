/*
 * main.cpp  —  MacroConverter 主程序入口（框架桩）
 *
 * 目前仅演示插件加载流程，UI 集成（DuiLib）待后续实现。
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>


#include "Manager/AppManager.h"

// ============================================================================
//  wWinMain：Windows GUI 程序入口
//  （CMakeLists.txt 中设置了 WIN32，所以入口是 WinMain 而非 main）
// ============================================================================

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // ── 调试期间附加控制台，方便查看日志 ────────────────────────────────────
#ifdef _DEBUG
    AllocConsole();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
#endif

    return DolPP::AppManager::GetInstance().Run(hInstance, nCmdShow);



    return 0;
}

#pragma once

#include "string_conv.h" // 引用你定义的 tstring 和转换逻辑
#include <windows.h>
#include <filesystem>
#include <tchar.h>

namespace Utils
{

/**
 * @brief 获取当前可执行文件 (.exe) 所在的目录
 * @return 返回工程匹配的 tstring (Unicode 下为 wstring，ANSI 下为 string)
 */

inline std::string GetExeDirA()
{
	std::string cacheDir;
	if (cacheDir.empty()) {
		char szPath[MAX_PATH] = { 0 };
		// GetModuleFileName 会根据是否定义 UNICODE 自动展开为 GetModuleFileNameW 或 GetModuleFileNameA
		if (::GetModuleFileNameA(NULL, szPath, MAX_PATH) > 0) {
			// 使用 std::filesystem 处理路径，它能很好地处理宽字符和窄字符的转换
			std::filesystem::path p(szPath);
			cacheDir = p.parent_path().string();
		}
	}
	return cacheDir;
}
inline std::wstring GetExeDirW()
{
	std::wstring cacheDir;
	if (cacheDir.empty()) {
		wchar_t szPath[MAX_PATH] = { 0 };
		// GetModuleFileName 会根据是否定义 UNICODE 自动展开为 GetModuleFileNameW 或 GetModuleFileNameA
		if (::GetModuleFileNameW(NULL, szPath, MAX_PATH) > 0) {
			// 使用 std::filesystem 处理路径，它能很好地处理宽字符和窄字符的转换
			std::filesystem::path p(szPath);
			cacheDir = p.parent_path().wstring();
		}
	}
	return cacheDir;
}


inline tstring GetExeDir()
{
	static tstring cacheDir;
	if (cacheDir.empty()) {
		tchar szPath[MAX_PATH] = { 0 };
		// GetModuleFileName 会根据是否定义 UNICODE 自动展开为 GetModuleFileNameW 或 GetModuleFileNameA
		if (::GetModuleFileName(NULL, szPath, MAX_PATH) > 0) {
			// 使用 std::filesystem 处理路径，它能很好地处理宽字符和窄字符的转换
			std::filesystem::path p(szPath);

#ifdef _UNICODE
			cacheDir = p.parent_path().wstring();
#else
			cacheDir = p.parent_path().string();
#endif
		}
	}
	return cacheDir;
}

/**
 * @brief 在 EXE 目录下拼接子路径
 */

inline tstring GetAppPath(const tstring& subPath)
{
	std::filesystem::path root(GetExeDir());
	std::filesystem::path full = root / subPath;

#ifdef _UNICODE
	return full.wstring();
#else
	return full.string();
#endif
}

/**
 * @brief 检查路径是否存在
 */
inline bool FileExists(const tstring& path)
{
	return std::filesystem::exists(path);
}

// 示例：如果 DuiLib 需要末尾带斜杠
inline tstring GetExeDirWithSlash()
{
	tstring dir = GetExeDir();
	if (!dir.empty() && dir.back() != _T('\\')) {
		dir += _T('\\');
	}
	return dir;
}

} // namespace Utils
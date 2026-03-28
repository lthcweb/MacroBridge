#pragma once

#include <string>
#include <windows.h>

namespace Utils
{
// 根据是否定义 _UNICODE，选择 tchar 和 tstring 类型
#ifdef _UNICODE
using tchar = wchar_t;
using tstring = std::wstring;
#define tcout std::wcout
#else
using tchar = char;
using tstring = std::string;
#define tcout std::cout
#endif

// Lua / 游戏内存 / JSON 全是 UTF-8，而 UI（DuiLib/Win32）是 UTF-16

// --- 基础转换函数 (UTF-8 <-> UTF-16) ---
inline std::wstring utf8_to_utf16(const std::string& utf8)
{
	if (utf8.empty()) return L"";
	int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
	std::wstring utf16(size, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &utf16[0], size);
	return utf16;
}

inline std::string utf16_to_utf8(const std::wstring& utf16)
{
	if (utf16.empty()) return "";
	int size = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), (int)utf16.size(), nullptr, 0, nullptr, nullptr);
	std::string utf8(size, 0);
	WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), (int)utf16.size(), &utf8[0], size, nullptr, nullptr);
	return utf8;
}

// --- 业务逻辑转换 (DuiLib/Win32 适配) ---

	// 1. 强制转为 UTF-16 (不管当前工程是 ANSI 还是 Unicode，DuiLib 内部逻辑常用)
inline std::wstring ToWString(const std::string& utf8) { return utf8_to_utf16(utf8); }
inline std::wstring ToWString(const std::wstring& utf16) { return utf16; }

// 2. 强制转为 UTF-8 (用于传给 Lua/游戏内存/JSON)
inline std::string ToString(const std::wstring& utf16) { return utf16_to_utf8(utf16); }
inline std::string ToString(const std::string& utf8) { return utf8; }

// 3. 转为工程匹配的 tstring (用于 Win32 API 或 DuiLib 控件赋值)
inline tstring ToTString(const std::string& utf8)
{
#ifdef _UNICODE
	return utf8_to_utf16(utf8);
#else
	return utf8; // 此时假设 std::string 本身就是本地编码或不需要转
#endif
}

inline tstring ToTString(const std::wstring& utf16)
{
#ifdef _UNICODE
	return utf16;
#else
	return utf16_to_utf8(utf16);
#endif
}

// 4. 从 tstring 统一转回 std::string (UTF-8)
inline std::string FromTString(const tstring& ts)
{
#ifdef _UNICODE
	return utf16_to_utf8(ts);
#else
	return ts;
#endif
}
} // namespace Utils
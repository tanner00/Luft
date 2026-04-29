#if PLATFORM_WINDOWS

#include "Windows.hpp"
#include "Array.hpp"
#include "Error.hpp"
#include "String.hpp"

#include "WindowsDefine.hpp"
#include <windows.h>
#include "WindowsUndefine.hpp"

namespace Windows
{

Array<wchar_t> UTF8ToWide(StringView utf8, Allocator* allocator)
{
	CHECK(allocator);

	Array<wchar_t> result(allocator);
	if (utf8.IsEmpty())
	{
		result.Add(L'\0');
		return result;
	}

	const int utf8Length = static_cast<int>(utf8.GetLength());
	const int wideLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.GetData(), utf8Length, nullptr, 0);
	VERIFY(wideLength != 0, "Failed to convert UTF-8 string to wide string!");

	result.AddUninitialized(static_cast<usize>(wideLength) + 1);
	CHECK(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.GetData(), utf8Length, result.GetData(), wideLength) == wideLength);
	result.Last() = L'\0';

	return result;
}

String WideToUTF8(const wchar_t* wide, Allocator* allocator)
{
	CHECK(allocator);

	String result(allocator);

	if (wide == nullptr)
	{
		return result;
	}

	const int wideLength = static_cast<int>(wcslen(wide));
	if (wideLength == 0)
	{
		return result;
	}

	const int utf8Length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, wideLength, nullptr, 0, nullptr, nullptr);
	VERIFY(utf8Length != 0, "Failed to convert wide string to UTF-8 string!");

	result.AddUninitialized(utf8Length);
	CHECK(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, wideLength, result.GetData(), utf8Length, nullptr, nullptr) == utf8Length);

	return result;
}

}

#endif

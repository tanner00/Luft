#if WINDOWS

#include "Allocator.hpp"
#include "Base.hpp"
#include "Error.hpp"
#include "Platform.hpp"

#undef UNICODE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

using MessageHandler = bool(*)(void*, uint32, uint64, uint64);
bool NoopMessageHandler(void*, uint32, uint64, uint64)
{
	return false;
}

static MessageHandler MessageHandlerOverride = NoopMessageHandler;
static bool QuitRequested = false;

namespace Platform
{

void MemoryCopy(void* destination, const void* source, usize size)
{
	memcpy(destination, source, size);
}

void MemoryMove(void* destination, const void* source, usize size)
{
	memmove(destination, source, size);
}

bool StringCompare(const char* a, usize aLength, const char* b, usize bLength)
{
	const usize maxLength = aLength > bLength ? aLength : bLength;
	const bool areEqual = strncmp(a, b, maxLength) == 0;
	return areEqual;
}

void* Allocate(usize size)
{
	const HANDLE heap = GetProcessHeap();
	CHECK(heap);
	void* ptr = HeapAlloc(heap, 0, size);
	CHECK(ptr);
	return ptr;
}

void Deallocate(void* ptr)
{
	const HANDLE heap = GetProcessHeap();
	CHECK(heap);
	const BOOL result = HeapFree(heap, 0, ptr);
	CHECK(result);
}

usize StringLength(const char* sNullTerminated)
{
	return strlen(sNullTerminated);
}

void StringPrint(const char* formatNullTerminated, char* buffer, usize bufferSize, ...)
{
	va_list args;
	va_start(args, bufferSize);
	const int result = vsnprintf(buffer, bufferSize, formatNullTerminated, args);
	CHECK(result);
	va_end(args);
}

void Log(const char* messageNullTerminated)
{
	OutputDebugStringA(messageNullTerminated);
}

uint8* ReadEntireFile(const char* filePathNullTerminated, usize* outSize)
{
	CHECK(outSize);

	const DWORD fileAttributes = GetFileAttributesA(filePathNullTerminated);
	const bool fileExists = fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	VERIFY(fileExists, "Attempted to open a file that doesn't exist!");

	const HANDLE file = CreateFileA(filePathNullTerminated, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	VERIFY(file, "Failed to open file!");

	LARGE_INTEGER fileSize = {};
	CHECK(SUCCEEDED(GetFileSizeEx(file, &fileSize)));
	CHECK(fileSize.HighPart == 0);
	*outSize = fileSize.LowPart;

	uint8* fileData = static_cast<uint8*>(GlobalAllocate(fileSize.LowPart));
	CHECK(fileData);

	DWORD readSize = 0;
	CHECK(SUCCEEDED(ReadFile(file, fileData, fileSize.LowPart, &readSize, nullptr)));
	VERIFY(fileSize.LowPart == readSize, "Failed to read entire file!");

	CHECK(SUCCEEDED(CloseHandle(file)));
	return fileData;
}

void FatalError(const char* errorMessageNullTerminated)
{
	MessageBoxA(nullptr, errorMessageNullTerminated, "Fatal Error!", MB_ICONERROR);
	ExitProcess(1);
}

bool IsQuitRequested()
{
	return QuitRequested;
}

void ProcessEvents()
{
	MSG msg;
	while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			QuitRequested = true;
		}

		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
}

static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (MessageHandlerOverride(window, message, wParam, lParam))
	{
		return true;
	}

	if (message == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcA(window, message, wParam, lParam);
}

Window MakeWindow(const char* name, int32 drawWidth, int32 drawHeight)
{
	BOOL result = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	CHECK(result);

	const HMODULE instance = GetModuleHandleA(nullptr);

	const usize windowClassNameLength = strlen(name) + sizeof(" Window Class") + sizeof('\0');
	char* windowClassName = static_cast<char*>(GlobalAllocate(windowClassNameLength));
	const int printResult = sprintf_s(windowClassName, windowClassNameLength, "%s Window Class", name);
	CHECK(SUCCEEDED(printResult));

	const WNDCLASSEXA windowClass =
	{
		.cbSize = sizeof(windowClass),
		.lpfnWndProc = WindowProc,
		.hInstance = instance,
		.hIcon = LoadIconA(nullptr, IDI_APPLICATION),
		.hCursor = LoadCursorA(nullptr, IDC_ARROW),
		.lpszClassName = windowClassName,
	};
	const ATOM atom = RegisterClassExA(&windowClass);
	CHECK(atom);

	constexpr DWORD exStyle = WS_EX_APPWINDOW;
	constexpr DWORD style = WS_OVERLAPPEDWINDOW & (~WS_THICKFRAME & ~WS_MAXIMIZEBOX);

	RECT windowRect = { 0, 0, drawWidth, drawHeight };
	AdjustWindowRectExForDpi(&windowRect, style, FALSE, exStyle, GetDpiForSystem());

	const HWND window = CreateWindowExA(exStyle, windowClass.lpszClassName, name, style,
										0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
										nullptr, nullptr, instance, nullptr);
	CHECK(window);

	const HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
	CHECK(monitor);
	MONITORINFO monitorInfo =
	{
		.cbSize = sizeof(monitorInfo),
	};
	result = GetMonitorInfoA(monitor, &monitorInfo);
	CHECK(result);
	const int32 windowPositionX = (monitorInfo.rcWork.left + monitorInfo.rcWork.right) / 2 - drawWidth / 2;
	const int32 windowPositionY = (monitorInfo.rcWork.top + monitorInfo.rcWork.bottom) / 2 - drawHeight / 2;
	SetWindowPos(window, nullptr, windowPositionX, windowPositionY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	return Window { window, windowClassName, drawWidth, drawHeight };
}

void DestroyWindow(Window& window)
{
	DestroyWindow(static_cast<HWND>(window.Handle));
	UnregisterClassA(static_cast<char*>(window.OsExtra), GetModuleHandleA(nullptr));
	GlobalDeallocate(window.OsExtra);
}

void ShowWindow(Window& window)
{
	ShowWindow(static_cast<HWND>(window.Handle), SW_SHOWNORMAL);
}

}

extern void Start();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	Start();
}

#endif

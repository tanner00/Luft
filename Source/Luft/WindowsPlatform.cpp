#if WINDOWS

#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")

#include "Allocator.hpp"
#include "Base.hpp"
#include "Error.hpp"
#include "HashTable.hpp"
#include "Platform.hpp"

#undef UNICODE
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

static bool NoOpMessageHandler(void*, uint32, uint64, uint64)
{
	return false;
}

static void NoOpResizeHandler(Platform::Window*)
{
}

static Platform::MessageHandler MessageHandlerOverride = NoOpMessageHandler;
static Platform::ResizeHandler ResizeHandlerOverride = NoOpResizeHandler;
static bool QuitRequested = false;

static HashTable<uint16, Key> WindowsKeyMap(32);
static bool KeyPressed[static_cast<usize>(Key::Count)] = {};
static bool KeyPressedOnce[static_cast<usize>(Key::Count)] = {};

static bool MouseButtonPressed[static_cast<usize>(MouseButton::Count)] = {};
static bool MouseButtonPressedOnce[static_cast<usize>(MouseButton::Count)] = {};
static int32 MouseX = 0;
static int32 MouseY = 0;

bool IsKeyPressed(Key key)
{
	CHECK(key != Key::Count);
	return KeyPressed[static_cast<usize>(key)];
}

bool IsKeyPressedOnce(Key key)
{
	CHECK(key != Key::Count);
	return KeyPressedOnce[static_cast<usize>(key)];
}

bool IsMouseButtonPressed(MouseButton button)
{
	CHECK(button != MouseButton::Count);
	return MouseButtonPressed[static_cast<usize>(button)];
}

bool IsMouseButtonPressedOnce(MouseButton button)
{
	CHECK(button != MouseButton::Count);
	return MouseButtonPressedOnce[static_cast<usize>(button)];
}

int32 GetMouseX()
{
	return MouseX;
}

int32 GetMouseY()
{
	return MouseY;
}

namespace Platform
{

void MemorySet(void* destination, uint8 value, usize size)
{
	memset(destination, value, size);
}

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

uint8* ReadEntireFile(const char* filePath, usize filePathSize, usize* outSize, Allocator& allocator)
{
	CHECK(outSize);

	char filePathNullTerminated[MAX_PATH] = {};
	VERIFY(filePathSize + 1 <= sizeof(filePathNullTerminated), "File path exceeds length limit!");
	MemoryCopy(filePathNullTerminated, filePath, filePathSize);

	const DWORD fileAttributes = GetFileAttributesA(filePathNullTerminated);
	const bool fileExists = fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	VERIFY(fileExists, "Attempted to open a file that doesn't exist!");

	const HANDLE file = CreateFileA(filePathNullTerminated, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	VERIFY(file, "Failed to open file!");

	LARGE_INTEGER fileSize = {};
	CHECK(SUCCEEDED(GetFileSizeEx(file, &fileSize)));
	CHECK(fileSize.HighPart == 0);
	*outSize = fileSize.LowPart;

	uint8* fileData = static_cast<uint8*>(allocator.Allocate(fileSize.LowPart));
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
	for (bool& justPressed : KeyPressedOnce)
	{
		justPressed = false;
	}
	for (bool& justPressed : MouseButtonPressedOnce)
	{
		justPressed = false;
	}

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

	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
	{
		Window* userWindow = reinterpret_cast<Window*>(GetWindowLongPtrA(window, 0));
		CHECK(userWindow);
		userWindow->DrawWidth = LOWORD(lParam);
		userWindow->DrawHeight = HIWORD(lParam);
		ResizeHandlerOverride(userWindow);
		return 0;
	}
	case WM_KEYDOWN:
	{
		const uint16 key = LOWORD(wParam);
		if (WindowsKeyMap.Contains(key))
		{
			const usize keyIndex = static_cast<usize>(WindowsKeyMap[key]);

			if (!KeyPressed[keyIndex])
			{
				KeyPressedOnce[keyIndex] = true;
			}
			KeyPressed[keyIndex] = true;
		}
		break;
	}
	case WM_KEYUP:
	{
		const uint16 key = LOWORD(wParam);
		if (WindowsKeyMap.Contains(key))
		{
			const usize keyIndex = static_cast<usize>(WindowsKeyMap[key]);
			KeyPressed[keyIndex] = false;
			KeyPressedOnce[keyIndex] = false;
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		constexpr usize button = static_cast<usize>(MouseButton::Left);
		if (!MouseButtonPressed[button])
		{
			MouseButtonPressedOnce[button] = true;
		}
		MouseButtonPressed[button] = true;
		break;
	}
	case WM_LBUTTONUP:
	{
		constexpr usize button = static_cast<usize>(MouseButton::Left);
		MouseButtonPressedOnce[button] = false;
		MouseButtonPressed[button] = false;
		break;
	}
	case WM_RBUTTONDOWN:
	{
		constexpr usize button = static_cast<usize>(MouseButton::Right);
		if (!MouseButtonPressed[button])
		{
			MouseButtonPressedOnce[button] = true;
		}
		MouseButtonPressed[button] = true;
		break;
	}
	case WM_RBUTTONUP:
	{
		constexpr usize button = static_cast<usize>(MouseButton::Right);
		MouseButtonPressedOnce[button] = false;
		MouseButtonPressed[button] = false;
		break;
	}
	case WM_MOUSEMOVE:
		MouseX = static_cast<int32>(LOWORD(lParam));
		MouseY = static_cast<int32>(HIWORD(lParam));
		break;
	default:
		break;
	}
	return DefWindowProcA(window, message, wParam, lParam);
}

Window* MakeWindow(const char* name, uint32 drawWidth, uint32 drawHeight)
{
	BOOL result = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	CHECK(result);

	const HMODULE instance = GetModuleHandleA(nullptr);

	const usize windowClassNameLength = strlen(name) + (sizeof(" Window Class") - 1) + (sizeof('\0') - 1) + 1;
	char* windowClassName = static_cast<char*>(GlobalAllocator::Get().Allocate(windowClassNameLength));
	const int printResult = sprintf_s(windowClassName, windowClassNameLength, "%s Window Class", name);
	CHECK(SUCCEEDED(printResult));

	const WNDCLASSEXA windowClass =
	{
		.cbSize = sizeof(windowClass),
		.lpfnWndProc = WindowProc,
		.cbWndExtra = sizeof(Window*),
		.hInstance = instance,
		.hIcon = LoadIconA(nullptr, IDI_APPLICATION),
		.hCursor = LoadCursorA(nullptr, IDC_ARROW),
		.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)),
		.lpszClassName = windowClassName,
	};
	const ATOM atom = RegisterClassExA(&windowClass);
	CHECK(atom);

	constexpr DWORD exStyle = WS_EX_APPWINDOW;
	constexpr DWORD style = WS_OVERLAPPEDWINDOW;

	RECT windowRect = { 0, 0, static_cast<int32>(drawWidth), static_cast<int32>(drawHeight) };
	AdjustWindowRectExForDpi(&windowRect, style, FALSE, exStyle, GetDpiForSystem());

	const HWND window = CreateWindowExA(exStyle, windowClass.lpszClassName, name, style,
										0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
										nullptr, nullptr, instance, nullptr);
	CHECK(window);

	Window* userWindow = GlobalAllocator::Get().Create<Window>(window, windowClassName, drawWidth, drawHeight);
	CHECK(SUCCEEDED(SetWindowLongPtrA(window, 0, reinterpret_cast<int64>(userWindow))));

	const HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
	CHECK(monitor);
	MONITORINFO monitorInfo =
	{
		.cbSize = sizeof(monitorInfo),
	};
	result = GetMonitorInfoA(monitor, &monitorInfo);
	CHECK(result);
	const int32 windowPositionX = (monitorInfo.rcWork.left + monitorInfo.rcWork.right) / 2 - static_cast<int32>(drawWidth) / 2;
	const int32 windowPositionY = (monitorInfo.rcWork.top + monitorInfo.rcWork.bottom) / 2 - static_cast<int32>(drawHeight) / 2;
	SetWindowPos(window, nullptr, windowPositionX, windowPositionY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	return userWindow;
}

void DestroyWindow(Window* window)
{
	DestroyWindow(static_cast<HWND>(window->Handle));
	UnregisterClassA(static_cast<char*>(window->OsExtra), GetModuleHandleA(nullptr));
	GlobalAllocator::Get().Deallocate(window->OsExtra, strlen(static_cast<char*>(window->OsExtra)) + 1);
	GlobalAllocator::Get().Destroy(window);
}

void ShowWindow(const Window* window)
{
	ShowWindow(static_cast<HWND>(window->Handle), SW_SHOWNORMAL);
}

void SetWindowTitle(const Window* window, const char* title)
{
	SetWindowTextA(static_cast<HWND>(window->Handle), title);
}

void InstallMessageHandler(MessageHandler handler)
{
	MessageHandlerOverride = handler;
}

void InstallResizeHandler(ResizeHandler handler)
{
	ResizeHandlerOverride = handler;
}

}

extern void Start();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	for (uint16 c = '0'; c <= '9'; ++c)
	{
		WindowsKeyMap.Add(c, static_cast<Key>(c - '0'));
	}
	for (uint16 c = 'A'; c <= 'Z'; ++c)
	{
		WindowsKeyMap.Add(c, static_cast<Key>(c - 'A' + static_cast<usize>(Key::A)));
	}
	WindowsKeyMap.Add(VK_LEFT, Key::Left);
	WindowsKeyMap.Add(VK_RIGHT, Key::Right);
	WindowsKeyMap.Add(VK_UP, Key::Up);
	WindowsKeyMap.Add(VK_DOWN, Key::Down);
	WindowsKeyMap.Add(VK_ESCAPE, Key::Escape);
	WindowsKeyMap.Add(VK_BACK, Key::Backspace);
	WindowsKeyMap.Add(VK_SPACE, Key::Space);
	WindowsKeyMap.Add(VK_RETURN, Key::Enter);

	const usize startingUsed = GlobalAllocator::Get().GetUsed();

	Start();

	const usize endingUsed = GlobalAllocator::Get().GetUsed();
	CHECK(endingUsed - startingUsed == 0);

	return 0;
}

#endif

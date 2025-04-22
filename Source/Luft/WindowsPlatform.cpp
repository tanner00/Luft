#if PLATFORM_WINDOWS

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

static uint64 Frequency;

static HashTable<uint16, Key> WindowsKeyMap(32, &GlobalAllocator::Get());
static bool KeyPressed[static_cast<usize>(Key::Count)] = {};
static bool KeyPressedOnce[static_cast<usize>(Key::Count)] = {};

static bool MouseButtonPressed[static_cast<usize>(MouseButton::Count)] = {};
static bool MouseButtonPressedOnce[static_cast<usize>(MouseButton::Count)] = {};

static int32 MouseX = 0;
static int32 MouseY = 0;

static InputMode CurrentInputMode = InputMode::Default;

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

usize StringLength(const char* string0)
{
	return strlen(string0);
}

void StringPrint(const char* format0, char* buffer, usize bufferSize, ...)
{
	va_list args;
	va_start(args, bufferSize);
	const int32 result = vsnprintf(buffer, bufferSize, format0, args);
	CHECK(result);
	va_end(args);
}

void Log(const char* message0)
{
	OutputDebugStringA(message0);
}

void LogFormatted(const char* format0, ...)
{
	char logBuffer[4096];

	va_list args;
	va_start(args, format0);
	const int32 result = vsnprintf(logBuffer, sizeof(logBuffer), format0, args);
	CHECK(result);
	va_end(args);

	Log(logBuffer);
}

uint8* ReadEntireFile(const char* filePath, usize filePathSize, usize* outSize, Allocator& allocator)
{
	CHECK(outSize);

	char filePath0[MAX_PATH] = {};
	VERIFY(filePathSize + 1 <= sizeof(filePath0), "File path exceeds length limit!");
	MemoryCopy(filePath0, filePath, filePathSize);

	const DWORD fileAttributes = GetFileAttributesA(filePath0);
	const bool fileExists = fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	VERIFY(fileExists, "Attempted to open a file that doesn't exist!");

	const HANDLE file = CreateFileA(filePath0, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
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

double GetTime()
{
	uint64 time;
	CHECK(SUCCEEDED(QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&time))));
	return static_cast<double>(time) / static_cast<double>(Frequency);
}

void FatalError(const char* errorMessage0)
{
	MessageBoxA(nullptr, errorMessage0, "Fatal Error!", MB_ICONERROR);
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
		return 0;
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
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		static constexpr usize button = static_cast<usize>(MouseButton::Left);
		if (!MouseButtonPressed[button])
		{
			MouseButtonPressedOnce[button] = true;
		}
		MouseButtonPressed[button] = true;
		return 0;
	}
	case WM_LBUTTONUP:
	{
		static constexpr usize button = static_cast<usize>(MouseButton::Left);
		MouseButtonPressedOnce[button] = false;
		MouseButtonPressed[button] = false;
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		static constexpr usize button = static_cast<usize>(MouseButton::Right);
		if (!MouseButtonPressed[button])
		{
			MouseButtonPressedOnce[button] = true;
		}
		MouseButtonPressed[button] = true;
		return 0;
	}
	case WM_RBUTTONUP:
	{
		static constexpr usize button = static_cast<usize>(MouseButton::Right);
		MouseButtonPressedOnce[button] = false;
		MouseButtonPressed[button] = false;
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		MouseX = static_cast<int32>(LOWORD(lParam));
		MouseY = static_cast<int32>(HIWORD(lParam));

		if (CurrentInputMode == InputMode::Captured)
		{
			const Window* userWindow = reinterpret_cast<Window*>(GetWindowLongPtrA(window, 0));

			POINT centerScreenPosition =
			{
				static_cast<int32>(userWindow->DrawWidth) / 2,
				static_cast<int32>(userWindow->DrawHeight) / 2,
			};

			MouseX -= centerScreenPosition.x;
			MouseY -= centerScreenPosition.y;

			ClientToScreen(window, &centerScreenPosition);
			SetCursorPos(centerScreenPosition.x, centerScreenPosition.y);
		}
		return 0;
	}
	case WM_KILLFOCUS:
		MemorySet(KeyPressed, false, sizeof(KeyPressed));
		MemorySet(KeyPressedOnce, false, sizeof(KeyPressedOnce));
		MemorySet(MouseButtonPressed, false, sizeof(MouseButtonPressed));
		MemorySet(MouseButtonPressedOnce, false, sizeof(MouseButtonPressedOnce));
		return 0;
	}
	return DefWindowProcA(window, message, wParam, lParam);
}

Window* MakeWindow(const char* name, uint32 drawWidth, uint32 drawHeight)
{
	BOOL result = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	CHECK(result);

	const HMODULE instance = GetModuleHandleA(nullptr);

	const usize windowClassNameLength = StringLength(name) + (sizeof(" Window Class") - 1) + (sizeof('\0') - 1) + 1;
	char* windowClassName = static_cast<char*>(GlobalAllocator::Get().Allocate(windowClassNameLength));
	StringPrint("%s Window Class", windowClassName, windowClassNameLength, name);

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

	static constexpr DWORD exStyle = WS_EX_APPWINDOW;
	static constexpr DWORD style = WS_OVERLAPPEDWINDOW;

	RECT windowRectangle = { 0, 0, static_cast<int32>(drawWidth), static_cast<int32>(drawHeight) };
	AdjustWindowRectExForDpi(&windowRectangle, style, FALSE, exStyle, GetDpiForSystem());

	const HWND window = CreateWindowExA(exStyle, windowClass.lpszClassName, name, style,
										0, 0, windowRectangle.right - windowRectangle.left, windowRectangle.bottom - windowRectangle.top,
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

bool IsWindowFocused(const Window* window)
{
	return GetActiveWindow() == static_cast<HWND>(window->Handle);
}

InputMode GetInputMode()
{
	return CurrentInputMode;
}

void SetInputMode(const Window* window, InputMode mode)
{
	if (mode == CurrentInputMode)
	{
		return;
	}

	switch (mode)
	{
	case InputMode::Default:
	{
		ClipCursor(nullptr);
		ShowCursor(true);
		break;
	}
	case InputMode::Captured:
	{
		const HWND nativeWindow = static_cast<HWND>(window->Handle);

		RECT screenRectangle;
		GetClientRect(nativeWindow, &screenRectangle);
		ClientToScreen(nativeWindow, reinterpret_cast<POINT*>(&screenRectangle.left));
		ClientToScreen(nativeWindow, reinterpret_cast<POINT*>(&screenRectangle.right));
		ClipCursor(&screenRectangle);

		ShowCursor(false);

		const int32 screenCenterX = (screenRectangle.left + screenRectangle.right) / 2;
		const int32 screenCenterY = (screenRectangle.top + screenRectangle.bottom) / 2;
		SetCursorPos(screenCenterX, screenCenterY);

		MouseX = 0;
		MouseY = 0;

		break;
	}
	}

	CurrentInputMode = mode;
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
	CHECK(SUCCEEDED(QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&Frequency))));

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
	WindowsKeyMap.Add(VK_SHIFT, Key::Shift);

	const usize startingUsed = GlobalAllocator::Get().GetUsed();

	Start();

	const usize endingUsed = GlobalAllocator::Get().GetUsed();
	CHECK(endingUsed - startingUsed == 0);

	return 0;
}

#endif

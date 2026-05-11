#if PLATFORM_WINDOWS

#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")
#pragma comment(lib, "shell32")

#include "Allocator.hpp"
#include "Base.hpp"
#include "Error.hpp"
#include "HashTable.hpp"
#include "Platform.hpp"
#include "Windows.hpp"

#include "WindowsDefine.hpp"
#include <windows.h>
#include <shellapi.h>
#include "WindowsUndefine.hpp"

#include <stdio.h>

#define GET_NATIVE_WINDOW(window) *static_cast<HWND*>((window)->Native)

namespace Platform
{

static uint64 Frequency = 0;

static bool QuitRequested = false;

static HashTable<uint16, Key> KeyMap(32, &GlobalAllocator::Get());
static bool KeyPressed[static_cast<usize>(Key::Count)] = {};
static bool KeyPressedOnce[static_cast<usize>(Key::Count)] = {};

static bool MouseButtonPressed[static_cast<usize>(MouseButton::Count)] = {};
static bool MouseButtonPressedOnce[static_cast<usize>(MouseButton::Count)] = {};

static int32 MouseX = 0;
static int32 MouseY = 0;

static InputMode CurrentInputMode = InputMode::Default;

static MessageHandler MessageHandlerOverride = [](void*, uint32, uint64, uint64) -> bool { return false; };
static ResizeHandler ResizeHandlerOverride = [](Window*) -> void {};

void MemorySet(void* destination, uint8 value, usize size)
{
	if (size == 0)
	{
		return;
	}

	CHECK(destination);
	memset(destination, value, size);
}

void MemoryCopy(void* destination, const void* source, usize size)
{
	if (size == 0)
	{
		return;
	}

	CHECK(destination);
	CHECK(source);
	memcpy(destination, source, size);
}

void MemoryMove(void* destination, const void* source, usize size)
{
	if (size == 0)
	{
		return;
	}

	CHECK(destination);
	CHECK(source);
	memmove(destination, source, size);
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

bool StringCompare(const char* a, usize aLength, const char* b, usize bLength)
{
	const bool areEqual = strncmp(a, b, aLength > bLength ? bLength : aLength) == 0;
	return areEqual;
}

usize StringLength(const char* string0)
{
	CHECK(string0);
	return strlen(string0);
}

void StringPrint(const char* format0, char* buffer, usize bufferSize, ...)
{
	CHECK(format0);
	CHECK(buffer);

	va_list args;
	va_start(args, bufferSize);
	const int32 result = vsnprintf(buffer, bufferSize, format0, args);
	CHECK(result);
	va_end(args);
}

void FatalError(const char* errorMessage0)
{
	CHECK(errorMessage0);
	const Array<wchar_t> errorMessageWide = Windows::UTF8ToWide(StringView(errorMessage0, StringLength(errorMessage0)));
	CHECK(MessageBoxW(nullptr, errorMessageWide.GetData(), L"Fatal Error!", MB_ICONERROR));
	ExitProcess(1);
}

void Log(const char* message0)
{
	CHECK(message0);
	const Array<wchar_t> messageWide = Windows::UTF8ToWide(StringView(message0, StringLength(message0)));
	OutputDebugStringW(messageWide.GetData());
}

void LogFormatted(const char* format0, ...)
{
	CHECK(format0);

	char buffer[4096];

	va_list args;
	va_start(args, format0);
	const int32 result = vsnprintf(buffer, sizeof(buffer), format0, args);
	CHECK(result);
	va_end(args);

	Log(buffer);
}

Array<String> GetCommandLineArguments(Allocator* allocator)
{
	CHECK(allocator);

	int32 argumentsCount = 0;
	wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &argumentsCount);
	if (arguments == nullptr)
	{
		return Array<String>(allocator);
	}

	Array<String> result(argumentsCount - 1, allocator);
	for (int32 index = 1; index < argumentsCount; ++index)
	{
		result.Add(Windows::WideToUTF8(arguments[index], allocator));
	}

	CHECK(LocalFree(arguments) == nullptr);

	return result;
}

uint8* ReadEntireFile(StringView filePath, usize* outSize, Allocator* allocator)
{
	CHECK(outSize);
	CHECK(allocator);

	const Array<wchar_t> filePathWide = Windows::UTF8ToWide(filePath);

	const uint64 fileAttributes = GetFileAttributesW(filePathWide.GetData());
	const bool fileExists = fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	VERIFY(fileExists, "Attempted to open a file that doesn't exist!");

	const HANDLE file = CreateFileW(filePathWide.GetData(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	VERIFY(file, "Failed to open file!");

	LARGE_INTEGER fileSize = {};
	CHECK(GetFileSizeEx(file, &fileSize));
	CHECK(fileSize.HighPart == 0);
	*outSize = fileSize.LowPart;

	uint8* fileData = static_cast<uint8*>(allocator->Allocate(fileSize.LowPart));
	CHECK(fileData);

	DWORD readSize = 0;
	CHECK(ReadFile(file, fileData, fileSize.LowPart, &readSize, nullptr));
	VERIFY(fileSize.LowPart == readSize, "Failed to read entire file!");

	CHECK(CloseHandle(file));
	return fileData;
}

float64 GetTime()
{
	uint64 time;
	CHECK(QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&time)));
	return static_cast<float64>(time) / static_cast<float64>(Frequency);
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
	while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			QuitRequested = true;
		}

		TranslateMessage(&msg);
		DispatchMessageW(&msg);
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
		Window* userWindow = reinterpret_cast<Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));
		CHECK(userWindow);
		userWindow->DrawWidth = LOWORD(lParam);
		userWindow->DrawHeight = HIWORD(lParam);
		ResizeHandlerOverride(userWindow);
		return 0;
	}
	case WM_KEYDOWN:
	{
		const uint16 key = LOWORD(wParam);
		if (KeyMap.Contains(key))
		{
			const usize keyIndex = static_cast<usize>(KeyMap[key]);

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
		if (KeyMap.Contains(key))
		{
			const usize keyIndex = static_cast<usize>(KeyMap[key]);
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
			const Window* userWindow = reinterpret_cast<Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));

			POINT centerScreenPosition =
			{
				static_cast<int32>(userWindow->DrawWidth) / 2,
				static_cast<int32>(userWindow->DrawHeight) / 2,
			};

			MouseX -= centerScreenPosition.x;
			MouseY -= centerScreenPosition.y;

			CHECK(ClientToScreen(window, &centerScreenPosition));
			CHECK(SetCursorPos(centerScreenPosition.x, centerScreenPosition.y));
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
	return DefWindowProcW(window, message, wParam, lParam);
}

Window* CreateWindow(StringView title, uint32 drawWidth, uint32 drawHeight)
{
	CHECK(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));

	const HMODULE instance = GetModuleHandleW(nullptr);

	String className(&GlobalAllocator::Get());
	className.Append(title);
	className.Append(" Window Class"_view);

	const Array<wchar_t> classNameWide = Windows::UTF8ToWide(className);

	void* native = GlobalAllocator::Get().Allocate(sizeof(HWND) + classNameWide.GetDataSize());

	wchar_t* classNamePersistentWide = reinterpret_cast<wchar_t*>(static_cast<uint8*>(native) + sizeof(HWND));
	MemoryCopy(classNamePersistentWide, classNameWide.GetData(), classNameWide.GetDataSize());

	const WNDCLASSEXW windowClass =
	{
		.cbSize = sizeof(windowClass),
		.style = 0,
		.lpfnWndProc = WindowProc,
		.cbWndExtra = sizeof(Window*),
		.hInstance = instance,
		.hIcon = LoadIconW(nullptr, IDI_APPLICATION),
		.hCursor = LoadCursorW(nullptr, IDC_ARROW),
		.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)),
		.lpszClassName = classNamePersistentWide,
	};
	CHECK(RegisterClassExW(&windowClass));

	static constexpr DWORD exStyle = WS_EX_APPWINDOW;
	static constexpr DWORD style = WS_OVERLAPPEDWINDOW;

	RECT windowRectangle = { 0, 0, static_cast<int32>(drawWidth), static_cast<int32>(drawHeight) };
	CHECK(AdjustWindowRectExForDpi(&windowRectangle, style, false, exStyle, GetDpiForSystem()));

	const Array<wchar_t> titleWide = Windows::UTF8ToWide(title);
	const HWND window = CreateWindowExW(exStyle, windowClass.lpszClassName, titleWide.GetData(), style,
										0, 0, windowRectangle.right - windowRectangle.left, windowRectangle.bottom - windowRectangle.top,
										nullptr, nullptr, instance, nullptr);
	CHECK(window);
	*static_cast<HWND*>(native) = window;

	Window* userWindow = GlobalAllocator::Get().Create<Window>(drawWidth, drawHeight, native);

	SetLastError(0);
	SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<uint64>(userWindow));
	CHECK(GetLastError() == 0);

	const HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
	CHECK(monitor);
	MONITORINFO monitorInfo =
	{
		.cbSize = sizeof(monitorInfo),
	};
	CHECK(GetMonitorInfoW(monitor, &monitorInfo));
	const int32 windowPositionX = (monitorInfo.rcWork.left + monitorInfo.rcWork.right) / 2 - static_cast<int32>(drawWidth) / 2;
	const int32 windowPositionY = (monitorInfo.rcWork.top + monitorInfo.rcWork.bottom) / 2 - static_cast<int32>(drawHeight) / 2;
	SetWindowPos(window, nullptr, windowPositionX, windowPositionY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

	return userWindow;
}

void DestroyWindow(Window* window)
{
	DestroyWindow(GET_NATIVE_WINDOW(window));
	const wchar_t* classNamePersistentWide = reinterpret_cast<wchar_t*>(static_cast<uint8*>(window->Native) + sizeof(HWND));
	UnregisterClassW(classNamePersistentWide, GetModuleHandleW(nullptr));
	GlobalAllocator::Get().Deallocate(window->Native, sizeof(HWND) + wcslen(classNamePersistentWide) * sizeof(wchar_t) + sizeof(L'\0'));
	GlobalAllocator::Get().Destroy(window);
}

void ShowWindow(const Window* window)
{
	ShowWindow(GET_NATIVE_WINDOW(window), SW_SHOWNORMAL);
}

void SetWindowTitle(const Window* window, StringView title)
{
	const Array<wchar_t> titleWide = Windows::UTF8ToWide(title);
	CHECK(SetWindowTextW(GET_NATIVE_WINDOW(window), titleWide.GetData()));
}

bool IsWindowFocused(const Window* window)
{
	return GetActiveWindow() == GET_NATIVE_WINDOW(window);
}

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
		CHECK(ClipCursor(nullptr));
		ShowCursor(true);
		break;
	}
	case InputMode::Captured:
	{
		const HWND nativeWindow = GET_NATIVE_WINDOW(window);

		RECT screenRectangle;
		CHECK(GetClientRect(nativeWindow, &screenRectangle));
		CHECK(ClientToScreen(nativeWindow, reinterpret_cast<POINT*>(&screenRectangle.left)));
		CHECK(ClientToScreen(nativeWindow, reinterpret_cast<POINT*>(&screenRectangle.right)));
		CHECK(ClipCursor(&screenRectangle));

		ShowCursor(false);

		const int32 screenCenterX = (screenRectangle.left + screenRectangle.right) / 2;
		const int32 screenCenterY = (screenRectangle.top + screenRectangle.bottom) / 2;
		CHECK(SetCursorPos(screenCenterX, screenCenterY));

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
	CHECK(QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&Platform::Frequency)));

	for (uint16 c = '0'; c <= '9'; ++c)
	{
		Platform::KeyMap.Add(c, static_cast<Platform::Key>(c - '0'));
	}
	for (uint16 c = 'A'; c <= 'Z'; ++c)
	{
		Platform::KeyMap.Add(c, static_cast<Platform::Key>(c - 'A' + static_cast<usize>(Platform::Key::A)));
	}
	Platform::KeyMap.Add(VK_LEFT, Platform::Key::Left);
	Platform::KeyMap.Add(VK_RIGHT, Platform::Key::Right);
	Platform::KeyMap.Add(VK_UP, Platform::Key::Up);
	Platform::KeyMap.Add(VK_DOWN, Platform::Key::Down);
	Platform::KeyMap.Add(VK_ESCAPE, Platform::Key::Escape);
	Platform::KeyMap.Add(VK_BACK, Platform::Key::Backspace);
	Platform::KeyMap.Add(VK_SPACE, Platform::Key::Space);
	Platform::KeyMap.Add(VK_RETURN, Platform::Key::Enter);
	Platform::KeyMap.Add(VK_SHIFT, Platform::Key::Shift);

	Start();

	return 0;
}

#endif

#pragma once

#include "Base.hpp"

#if PLATFORM_WINDOWS
#define BREAK_IN_DEBUGGER __debugbreak
#else
#error "The platform layer is currently unimplemented for this target!"
#endif

class Allocator;

enum class InputMode : uint8
{
	Default,
	Captured,
};

enum class Key : uint8
{
	Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine,
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
	Left, Right, Up, Down,
	Escape, Backspace, Space, Enter, Shift,
	Count,
};

enum class MouseButton : uint8
{
	Left,
	Right,
	Count,
};

bool IsKeyPressed(Key key);
bool IsKeyPressedOnce(Key key);

bool IsMouseButtonPressed(MouseButton button);
bool IsMouseButtonPressedOnce(MouseButton button);
int32 GetMouseX();
int32 GetMouseY();

namespace Platform
{

struct Window
{
	void* Handle;
	void* OsExtra;

	uint32 DrawWidth;
	uint32 DrawHeight;
};

using MessageHandler = bool(*)(void*, uint32, uint64, uint64);
using ResizeHandler = void(*)(Window*);

void MemorySet(void* destination, uint8 value, usize size);
void MemoryCopy(void* destination, const void* source, usize size);
void MemoryMove(void* destination, const void* source, usize size);
bool StringCompare(const char* a, usize aLength, const char* b, usize bLength);

void* Allocate(usize size);
void Deallocate(void* ptr);

usize StringLength(const char* string0);
void StringPrint(const char* format0, char* buffer, usize bufferSize, ...);

void Log(const char* message0);
void LogFormatted(const char* format0, ...);
void FatalError(const char* errorMessage0);

uint8* ReadEntireFile(const char* filePath, usize filePathSize, usize* outSize, Allocator& allocator);

double GetTime();

bool IsQuitRequested();
void ProcessEvents();

Window* CreateWindow(const char* name, uint32 drawWidth, uint32 drawHeight);
void DestroyWindow(Window* window);
void ShowWindow(const Window* window);
void SetWindowTitle(const Window* window, const char* title);
bool IsWindowFocused(const Window* window);

InputMode GetInputMode();
void SetInputMode(const Window* window, InputMode mode);

void InstallMessageHandler(MessageHandler handler);
void InstallResizeHandler(ResizeHandler handler);

}

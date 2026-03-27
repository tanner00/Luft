#pragma once

#include "PlatformCore.hpp"
#include "Allocator.hpp"
#include "String.hpp"

namespace Platform
{

struct Window
{
	void* Handle;
	void* OSExtra;

	uint32 DrawWidth;
	uint32 DrawHeight;
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

enum class InputMode : uint8
{
	Default,
	Captured,
};

using MessageHandler = bool(*)(void*, uint32, uint64, uint64);
using ResizeHandler = void(*)(Window*);

void Log(const char* message0);
void LogFormatted(const char* format0, ...);

Array<String> GetCommandLineArguments(Allocator* allocator = &GlobalAllocator::Get());

uint8* ReadEntireFile(StringView filePath, usize* outSize, Allocator* allocator = &GlobalAllocator::Get());

double GetTime();

bool IsQuitRequested();
void ProcessEvents();

Window* CreateWindow(StringView title, uint32 drawWidth, uint32 drawHeight);
void DestroyWindow(Window* window);
void ShowWindow(const Window* window);
void SetWindowTitle(const Window* window, StringView title);
bool IsWindowFocused(const Window* window);

bool IsKeyPressed(Key key);
bool IsKeyPressedOnce(Key key);

bool IsMouseButtonPressed(MouseButton button);
bool IsMouseButtonPressedOnce(MouseButton button);
int32 GetMouseX();
int32 GetMouseY();

InputMode GetInputMode();
void SetInputMode(const Window* window, InputMode mode);

void InstallMessageHandler(MessageHandler handler);
void InstallResizeHandler(ResizeHandler handler);

}

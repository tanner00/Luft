#pragma once

#include "Base.hpp"

#if WINDOWS

#define BREAK_IN_DEBUGGER __debugbreak

#else
#error "The platform layer is currently unimplemented for this target!"
#endif

namespace Platform
{

struct Window
{
	void* Handle;
	void* OsExtra;

	int32 DrawWidth;
	int32 DrawHeight;
};

void MemoryCopy(void* destination, const void* source, usize size);
void MemoryMove(void* destination, const void* source, usize size);

void* Allocate(usize size);
void Deallocate(void* ptr);

usize StringLength(const char* s);
void StringPrint(const char* format, char* buffer, usize bufferSize, ...);
void Log(const char* message);

uint8* ReadEntireFile(const char* filePath, usize* outSize);

void FatalError(const char* errorMessage);

bool IsQuitRequested();
void ProcessEvents();

Window MakeWindow(const char* name, int32 drawWidth, int32 drawHeight);
void DestroyWindow(Window& window);
void ShowWindow(Window& window);

}

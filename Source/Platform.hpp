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

void FatalError(const char* errorMessage);

bool IsQuitRequested();
void ProcessEvents();

Window WindowCreate(const char* name, int32 drawWidth, int32 drawHeight);
void WindowDestroy(Window& window);
void WindowShow(Window& window);

}

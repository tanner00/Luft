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

using MessageHandler = bool(*)(void*, uint32, uint64, uint64);

void MemoryCopy(void* destination, const void* source, usize size);
void MemoryMove(void* destination, const void* source, usize size);
bool StringCompare(const char* a, usize aLength, const char* b, usize bLength);

void* Allocate(usize size);
void Deallocate(void* ptr);

usize StringLength(const char* sNullTerminated);
void StringPrint(const char* formatNullTerminated, char* buffer, usize bufferSize, ...);
void Log(const char* messageNullTerminated);

uint8* ReadEntireFile(const char* filePathNullTerminated, usize* outSize);

void FatalError(const char* errorMessageNullTerminated);

bool IsQuitRequested();
void ProcessEvents();

Window MakeWindow(const char* name, int32 drawWidth, int32 drawHeight);
void DestroyWindow(Window& window);
void ShowWindow(Window& window);

void InstallMessageHandler(MessageHandler handler);

}

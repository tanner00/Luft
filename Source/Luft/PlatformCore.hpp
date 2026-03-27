#pragma once

#include "Base.hpp"

#if PLATFORM_WINDOWS
#define BREAK_IN_DEBUGGER __debugbreak
#else
#error "The platform layer is currently unimplemented for this target!"
#endif

namespace Platform
{

void MemorySet(void* destination, uint8 value, usize size);
void MemoryCopy(void* destination, const void* source, usize size);
void MemoryMove(void* destination, const void* source, usize size);

void* Allocate(usize size);
void Deallocate(void* ptr);

bool StringCompare(const char* a, usize aLength, const char* b, usize bLength);
usize StringLength(const char* string0);

void StringPrint(const char* format0, char* buffer, usize bufferSize, ...);

void FatalError(const char* errorMessage0);

}

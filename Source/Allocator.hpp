#pragma once

#include "Meta.hpp"
#include "Platform.hpp"

struct LuftNewMarker
{
};

inline void* GlobalAllocate(usize size)
{
	return Platform::Allocate(size);
}

inline void GlobalDeallocate(void* ptr)
{
	Platform::Deallocate(ptr);
}

inline void* operator new(usize size, void* at, LuftNewMarker) noexcept
{
	(void)size;
	return at;
}

template<typename T, typename... Args>
T* GlobalCreate(Args&&... args)
{
	T* object = static_cast<T*>(GlobalAllocate(sizeof(T)));
	object = new (object, LuftNewMarker {}) T(Forward<Args>(args)...);
	return object;
}

template<typename T>
void GlobalDestroy(T* object)
{
	if (object)
	{
		object->~T();
	}
	GlobalDeallocate(object);
}

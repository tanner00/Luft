#pragma once

#include "Base.hpp"
#include "Meta.hpp"
#include "NoCopy.hpp"

struct LuftNewMarker
{
};

inline void* operator new(usize size, void* at, LuftNewMarker) noexcept
{
	(void)size;
	return at;
}

class Allocator : public NoCopy
{
public:
	virtual ~Allocator() = default;

	virtual void* Allocate(usize size) = 0;
	virtual void Deallocate(void* ptr, usize size) = 0;

	template<typename T, typename... Args>
	T* Create(Args&&... args)
	{
		T* object = static_cast<T*>(Allocate(sizeof(T)));
		object = new (object, LuftNewMarker {}) T { Forward<Args>(args)... };
		return object;
	}

	template<typename T>
	void Destroy(T* object)
	{
		if (object)
		{
			object->~T();
		}
		Deallocate(object, sizeof(T));
	}
};

class GlobalAllocator final : public Allocator
{
public:
	static GlobalAllocator& Get()
	{
		static GlobalAllocator instance;
		return instance;
	}

	usize GetUsed() const
	{
		return Used;
	}

	void* Allocate(usize size) override;
	void Deallocate(void* ptr, usize size) override;

private:
	GlobalAllocator() = default;

	usize Used = 0;
};

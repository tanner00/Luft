#include "Allocator.hpp"
#include "PlatformCore.hpp"

void* GlobalAllocator::Allocate(usize size)
{
	Used += size;
	return Platform::Allocate(size);
}

void GlobalAllocator::Deallocate(void* ptr, usize size)
{
	if (ptr)
	{
		Used -= size;
	}
	Platform::Deallocate(ptr);
}

void* StaticAllocator::Allocate(usize size)
{
	return Platform::Allocate(size);
}

void StaticAllocator::Deallocate(void* ptr, usize size)
{
	(void)size;
	Platform::Deallocate(ptr);
}

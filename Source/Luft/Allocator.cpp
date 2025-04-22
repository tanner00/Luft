#include "Allocator.hpp"
#include "Platform.hpp"

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

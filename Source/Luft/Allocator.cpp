#include "Allocator.hpp"
#include "Platform.hpp"

void* GlobalAllocator::Allocate(usize size)
{
	return Platform::Allocate(size);
}

void GlobalAllocator::Deallocate(void* ptr, usize size)
{
	(void)size;
	Platform::Deallocate(ptr);
}

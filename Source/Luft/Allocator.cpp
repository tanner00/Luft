#include "Allocator.hpp"
#include "PlatformCore.hpp"
#include "Error.hpp"

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

#if DEBUG
class GlobalAllocatorChecker : public NoCopy
{
public:
	GlobalAllocatorChecker()
		: StartUsed(GlobalAllocator::Get().GetUsed())
	{
	}

	~GlobalAllocatorChecker()
	{
		const usize endUsed = GlobalAllocator::Get().GetUsed();
		CHECK(endUsed - StartUsed == 0);
	}

	usize StartUsed;
};

#pragma init_seg(lib)
static GlobalAllocatorChecker GlobalAllocatorChecker;
#endif

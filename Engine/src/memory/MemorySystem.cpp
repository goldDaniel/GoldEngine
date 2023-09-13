#include "MemorySystem.h"

using namespace gold;

MemorySystem& gold::MemorySystem::Get()
{
	static MemorySystem sys;
	return sys;
}

void MemorySystem::Init()
{
	DEBUG_ASSERT(!mInitialized);
	
	mMemorySize = 4 * memory::GB;
	mMemory = (u8*)malloc(mMemorySize);

	mMemorySize -= sizeof(LinearAllocator);
	mGlobalAllocator = new(mMemory) LinearAllocator(mMemory + sizeof(LinearAllocator), mMemorySize);

	void* generalAllocatorMemory = mGlobalAllocator->Allocate(sizeof(FreeListAllocator));

	u64 remainingMemory = mGlobalAllocator->GetSize() - mGlobalAllocator->GetUsedMemory();
	void* generalMemory = mGlobalAllocator->Allocate(remainingMemory);
	mGeneralAllocator = new(generalAllocatorMemory) FreeListAllocator(generalMemory, remainingMemory);

	mInitialized = true;
}

void MemorySystem::Shutdown()
{
	free(mMemory);
}

void* operator new(std::size_t n) throw(std::bad_alloc)
{
	return gold::MemorySystem::Get().GetGeneralAllocator().Allocate(n);
}

void operator delete(void* p) throw()
{
	gold::MemorySystem::Get().GetGeneralAllocator().Deallocate(p);
}


void* operator new[](std::size_t n) throw(std::bad_alloc)
{
	return gold::MemorySystem::Get().GetGeneralAllocator().Allocate(n);
}

void operator delete[](void* p) throw()
{
	gold::MemorySystem::Get().GetGeneralAllocator().Deallocate(p);
}
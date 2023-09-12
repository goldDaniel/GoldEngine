#include "MemorySystem.h"

using namespace gold;



void MemorySystem::Init()
{
	DEBUG_ASSERT(!mInitialized);
	
	mMemorySize = 256 * MB;
	mMemory = malloc(mMemorySize);
	
	mGlobalAllocator = std::make_unique<LinearAllocator>(mMemory, mMemorySize);

	u8* generalMemory = mGlobalAllocator->Allocate(mMemorySize / 2);
	mGeneralAllocator = std::make_unique<FreeListAllocator>(generalMemory, mMemorySize / 2);

	u8* perFrameMemory = mGlobalAllocator->Allocate(mMemorySize / 4);
	mFrameAllocator = std::make_unique<LinearAllocator>(perFrameMemory, mMemorySize / 4);

	mInitialized = true;
}

void MemorySystem::Shutdown()
{
	FreeGlobalMemory();
	free(mMemory);
}

void* operator new[](std::size_t n) throw(std::bad_alloc)
{
	return mGeneralAllocator.Allocate(n);
}

void operator delete[](void* p) throw()
{
	mGeneralAllocator.Free(p);
}


void* operator new[](std::size_t n) throw(std::bad_alloc)
{
	return mGeneralAllocator.Allocate(n);
}

void operator delete[](void* p) throw()
{
	mGeneralAllocator.Free(p);
}
#include "Allocator.h"

gold::Allocator::Allocator(void* mem, u64 size)
	: mMemory(mem)
	, mSize(size)
	, mUsed(0)
	, mNumAllocations(0)
{
}

gold::Allocator::~Allocator()
{
	DEBUG_ASSERT(mNumAllocations == 0 && mUsed == 0, "Memory must be freed before destruction!");
	mMemory = nullptr;
	mSize = 0;
}

void gold::Allocator::Free()
{
	free(mMemory);
	mMemory = nullptr;
	mNumAllocations = 0;
	mUsed = 0;
	mSize = 0;
}

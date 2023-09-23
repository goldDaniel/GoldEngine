#include "LinearAllocator.h"

#include "Utils.h"

using namespace gold;
LinearAllocator::LinearAllocator(void* start, u64 size) 
	: Allocator(start, size)
	, mCurrentAddress(start) 
{ 
	DEBUG_ASSERT(size > 0, "size must be greater than 0");
	memset(mMemory, 0, mSize);
}

LinearAllocator::~LinearAllocator() 
{ 
	mCurrentAddress = nullptr; 
}

void* LinearAllocator::Allocate(size_t size, u8 alignment)
{
	DEBUG_ASSERT(size > 0, "size must be greater than 0");

	u8 adjustment = allocator::alignForwardAdjustment(mCurrentAddress, alignment);

	if (mUsed + adjustment + size > mSize)
	{
		return nullptr;
	}

	u8* aligned_address = (u8*)mCurrentAddress + adjustment;
	mCurrentAddress = (void*)(aligned_address + size);

	mUsed += size + adjustment;
	mNumAllocations++;

	return (void*)aligned_address;
}

void LinearAllocator::Deallocate(void* p)
{
	DEBUG_ASSERT(false, "Use Reset() instead");
}

void LinearAllocator::Reset()
{
	memset(mMemory, 0, mUsed);
	mNumAllocations = 0;
	mUsed = 0;
	mCurrentAddress = mMemory;
}
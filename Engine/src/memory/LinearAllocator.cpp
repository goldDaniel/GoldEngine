#include "LinearAllocator.h"

#include "Utils.h"

using namespace gold;

LinearAllocator::LinearAllocator(void* mem, const u64 totalSize)
	: Allocator(mem, totalSize) 
{
	Init();
}

LinearAllocator::~LinearAllocator()
{
	DEBUG_ASSERT(mInitialized);
	mInitialized = false;
}

void LinearAllocator::Init()
{
	DEBUG_ASSERT(!mInitialized);

	mOffset = 0;
	mInitialized = true;
}

void* LinearAllocator::Allocate(const u64 size, const u64 alignment)
{
	std::size_t padding = 0;
	std::size_t paddedAddress = 0;
	const std::size_t currentAddress = (std::size_t)mMemory + mOffset;

	if (alignment != 0 && mOffset % alignment != 0) 
	{
		padding = memory::CalculatePadding(currentAddress, alignment);
	}

	if (mOffset + padding + size > mAllocatorSize) 
	{
		return nullptr;
	}

	mOffset += padding;
	const std::size_t nextAddress = currentAddress + padding;
	mOffset += size;

	mUsed = mOffset;
	mPeak = mUsed > mPeak ? mUsed : mPeak;

	return (void*)nextAddress;
}

void LinearAllocator::Free(void* mem)
{
	// cannot free with linear allocator
}

void LinearAllocator::Reset() 
{
	mOffset = 0;
	mUsed = 0;
	mPeak = 0;
}
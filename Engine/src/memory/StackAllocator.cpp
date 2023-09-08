#include "StackAllocator.h"
#include "Utils.h"

using namespace gold;

namespace 
{
	struct AllocationHeader
	{
		uint8_t padding;
	};
}


StackAllocator::StackAllocator(void* mem, const u64 totalSize)
	: Allocator(mem, totalSize)
	, mOffset(0)
{
	Init();
}

StackAllocator::~StackAllocator()
{
	Reset();
}

void StackAllocator::Init()
{
	DEBUG_ASSERT(!mInitialized);
	DEBUG_ASSERT(mMemory != nullptr);
	DEBUG_ASSERT(mAllocatorSize > 0);

	mOffset = 0;
	mInitialized = true;
}

void* StackAllocator::Allocate(const u64 size, const u64 allignment)
{
	DEBUG_ASSERT(mInitialized);

	u64 address = (u64)mMemory + mOffset;
	u64 padding = memory::CalculatePaddingWithHeader<AllocationHeader>(address, allignment);

	DEBUG_ASSERT((mOffset + padding + size) <= mAllocatorSize);
	if ((mOffset + padding + size) > mAllocatorSize)
	{
		return nullptr;
	}

	mOffset += padding;

	const u64 nextAddress	= address + padding;
	const u64 headerAddress	= nextAddress - sizeof(AllocationHeader);
	
	AllocationHeader header{ static_cast<u8>(padding) };
	std::memcpy((void*)(headerAddress), &header, sizeof(AllocationHeader));

	mOffset += size;

	mUsed = mOffset;
	mPeak = mUsed > mPeak ? mUsed : mPeak;

	return (void*)nextAddress;
}

void StackAllocator::Free(void* mem)
{
	DEBUG_ASSERT(mInitialized);
	
	u64 address = (u64)mem;
	u64 headerAddress = address - sizeof(AllocationHeader);

	const AllocationHeader* header = (AllocationHeader*)headerAddress;
	mOffset = address - header->padding - (u64)mMemory;
	mUsed = mOffset;
}

void StackAllocator::Reset()
{
	mOffset = 0;
	mUsed = 0;
	mPeak = 0;
	mInitialized = false;
}

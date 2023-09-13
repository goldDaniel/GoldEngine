#include "FreeListAllocator.h"

using namespace gold;

struct AllocationHeader 
{ 
	u64 size; 
	u8 adjustment; 
};

FreeListAllocator::FreeListAllocator(void* start, u64 size) 
	: Allocator(start, size)
	, mFreeBlocks((FreeBlock*)start)
{
	STATIC_ASSERT(sizeof(AllocationHeader) >= sizeof(FreeListAllocator::FreeBlock), "sizeof(AllocationHeader) < sizeof(FreeBlock)");

	DEBUG_ASSERT(size > sizeof(FreeBlock));
	mFreeBlocks->size = size;
	mFreeBlocks->next = nullptr;
}

FreeListAllocator::~FreeListAllocator()
{
	mFreeBlocks = nullptr;
}

void* FreeListAllocator::Allocate(size_t size, u8 alignment)
{
	DEBUG_ASSERT(size > 0 && alignment > 0);
	FreeBlock* prevFreeBlock = nullptr;
	FreeBlock* currFreeBlock = mFreeBlocks;

	while (currFreeBlock)
	{
		//Calculate adjustment needed to keep object correctly aligned 
		u8 adjustment = allocator::alignForwardAdjustmentWithHeader(currFreeBlock, alignment, sizeof(AllocationHeader));
		size_t totalSize = size + adjustment;

		// allocation does not fit in current block, look at next
		if (currFreeBlock->size < totalSize)
		{
			prevFreeBlock = currFreeBlock;
			currFreeBlock = currFreeBlock->next;
			continue;
		}

		
		//If allocations in the remaining memory will be impossible 
		if (currFreeBlock->size - totalSize <= sizeof(AllocationHeader))
		{
			//Increase allocation size instead of creating a new FreeBlock 
			totalSize = currFreeBlock->size;

			if (prevFreeBlock)
			{
				prevFreeBlock->next = currFreeBlock->next;
			}
			else
			{
				mFreeBlocks = currFreeBlock->next;
			}
		}
		else
		{
			FreeBlock* nextBlock = (FreeBlock*)(currFreeBlock + totalSize);

			nextBlock->size = currFreeBlock->size - totalSize;
			nextBlock->next = currFreeBlock->next;

			if (prevFreeBlock)
			{
				prevFreeBlock->next = nextBlock;
			}
			else
			{
				mFreeBlocks = nextBlock;
			}
		}

		u8* aligned_address = (u8*)allocator::add(currFreeBlock, adjustment);
		AllocationHeader* header = (AllocationHeader*)(aligned_address - sizeof(AllocationHeader));
		header->size = totalSize;
		header->adjustment = adjustment;
		mUsed += totalSize;
		mNumAllocations++;

		DEBUG_ASSERT(allocator::alignForwardAdjustment((void*)aligned_address, alignment) == 0);

		return (void*)aligned_address;
	}

	DEBUG_ASSERT(false && "Couldn't find free block large enough!"); 
	return nullptr;
}

void FreeListAllocator::Deallocate(void* p)
{
	DEBUG_ASSERT(p);

	AllocationHeader* header = (AllocationHeader*)allocator::subtract(p, sizeof(AllocationHeader));
	
	u8* blockStart = reinterpret_cast<u8*>(p) - header->adjustment;
	size_t blockSize = header->size;
	u8* blockEnd = blockStart + blockSize;
	
	FreeBlock* prevFreeBlock = nullptr;
	FreeBlock* currFreeBlock= mFreeBlocks;

	while (currFreeBlock)
	{
		if ((u8*)currFreeBlock >= blockEnd) break;

		prevFreeBlock = currFreeBlock;
		currFreeBlock = currFreeBlock->next;
	}

	if (!prevFreeBlock)
	{
		prevFreeBlock = (FreeBlock*)blockStart;
		prevFreeBlock->size = blockSize;
		prevFreeBlock->next = mFreeBlocks;
		mFreeBlocks = prevFreeBlock;
	}
	else if ((u8*)prevFreeBlock + prevFreeBlock->size == blockStart)
	{
		prevFreeBlock->size += blockSize;
	}
	else
	{
		FreeBlock* temp = (FreeBlock*)blockStart;
		
		temp->size = blockSize;
		temp->next = prevFreeBlock->next;
		
		prevFreeBlock->next = temp;
		prevFreeBlock = temp;
	}

	if (currFreeBlock && (u8*)currFreeBlock == blockEnd)
	{
		prevFreeBlock->size += currFreeBlock->size;
		prevFreeBlock->next = currFreeBlock->next;
	}

	mNumAllocations--;
	mUsed -= blockSize;
}
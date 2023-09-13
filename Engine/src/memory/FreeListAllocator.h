#pragma once

#include "Allocator.h"

namespace gold
{
	class FreeListAllocator : public Allocator
	{
	public:

		FreeListAllocator(void* start, u64 size);
		FreeListAllocator(FreeListAllocator&&) = default;
		FreeListAllocator(const FreeListAllocator&) = delete;
		FreeListAllocator& operator=(const FreeListAllocator&) = delete;

		virtual ~FreeListAllocator() override;

		void* Allocate(size_t size, u8 alignment = 4) override;
		void Deallocate(void* p) override;

	private:

		struct FreeBlock 
		{ 
			u64 size; 
			FreeBlock* next; 
		};
		
		FreeBlock* mFreeBlocks;
	};
}
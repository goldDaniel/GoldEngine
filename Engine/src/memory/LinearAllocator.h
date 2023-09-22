#pragma once

#include "Allocator.h"

namespace gold
{
	class LinearAllocator : public Allocator
	{
	private:
		void* mCurrentAddress = nullptr;

	public:

		LinearAllocator(void* start, u64 size);
		
		LinearAllocator(const LinearAllocator&) = delete;
		LinearAllocator(LinearAllocator&&) = default;
		LinearAllocator& operator=(const LinearAllocator&) = delete;

		~LinearAllocator();

		void* Allocate(size_t size, u8 alignment = 4) override;
		void Deallocate(void* p) override;
		void Reset();
	};
}
#pragma once 

#include "Allocator.h"

namespace gold
{
	class StackAllocator : public Allocator
	{
	private:
		uint64_t mOffset;
	public:
		StackAllocator(void* mem, const u64 totalSize);

		StackAllocator(StackAllocator&) = delete;
		StackAllocator(StackAllocator&&) = delete;
		void operator=(StackAllocator&) = delete;

		virtual ~StackAllocator() override;

		virtual void* Allocate(const u64 size, const u64 allignment) override;

		virtual void Free(void* mem) override;

		virtual void Init() override;

		void Reset();
	};
}

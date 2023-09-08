#pragma once

#include "Allocator.h"

namespace gold
{
	class LinearAllocator : public Allocator
	{
	private:
		uint64_t mOffset;
	public:
		LinearAllocator(void* mem, const u64 totalSize);

		LinearAllocator(LinearAllocator&) = delete;
		LinearAllocator(LinearAllocator&&) = delete;
		void operator=(LinearAllocator&) = delete;

		virtual ~LinearAllocator() override;

		virtual void* Allocate(const u64 size, const u64 allignment) override;

		virtual void Free(void* mem) override;

		virtual void Init() override;

		void Reset();
	};
}
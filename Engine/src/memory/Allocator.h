#pragma once

#include <core/core.h>

namespace gold
{
	struct AllocatorStats
	{
		u64 mAllocatorSize;
		u64 mUsed;
		u64 mPeak;
	};

	class Allocator
	{
	protected:
		u64 mAllocatorSize;
		u64 mUsed;
		u64 mPeak;

		bool mInitialized;

		void* mMemory;
	public:
		Allocator(void* mem, const u64 allocatorSize)
			: mAllocatorSize(allocatorSize)
			, mUsed(0)
			, mPeak(0)
			, mInitialized(false)
			, mMemory(mem)
		{
		}

		virtual ~Allocator() { mAllocatorSize = 0; }

		virtual void Init() = 0;

		AllocatorStats GetStats() const;

		virtual void* Allocate(const u64 size, u64 allignment = 0) = 0;

		virtual void Free(void* mem) = 0;

		template<typename T>
		T* AllocateType(const u64 allignment = 0)
		{
			return static_cast<T*>(Allocate(sizeof(T), allignment));
		}
	};
}

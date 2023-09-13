#pragma once

#include "core/Core.h"

#include "LinearAllocator.h"
#include "FreeListAllocator.h"

namespace gold
{
	namespace memory
	{
		constexpr u64 KB = 1024;
		constexpr u64 MB = KB * 1024;
		constexpr u64 GB = MB * 1024;
	}

	class MemorySystem
	{
	private:
		u8* mMemory{};
		u64 mMemorySize{};
		bool mInitialized{};

		LinearAllocator* mGlobalAllocator;

		FreeListAllocator* mGeneralAllocator;
		
	public:
		static MemorySystem& Get();

		void Init();
		void Shutdown();

		inline Allocator& GetGeneralAllocator() { return *mGeneralAllocator; }
	};
}

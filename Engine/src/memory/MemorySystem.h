#pragma once

#include "core/Core.h"

namespace memory
{
	constexpr u64 KB = 1024;
	constexpr u64 MB = KB * 1024;
	constexpr u64 GB = MB * 1024;
}

namespace gold
{
	class MemorySystem
	{
	private:
		u8* mMemory{};
		u64 mMemorySize{};
		bool mInitialized{};

		std::unique_ptr<LinearAllocator> mGlobalAllocator;

		std::unique_ptr<LinearAllocator> mFrameAllocator;
		std::unique_ptr<FreeListAllocator> mGeneralAllocator;
		
	public:
		void Init();
		void Shutdown();

		Allocator& GetGeneralAllocator();
		Allocator& GetPerFrameAllocator();

		u8* AllocateSystemMemory(u64 memorySize);
		void FreeGlobalMemory();
	};
}

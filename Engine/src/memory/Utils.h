#pragma once 

#include "core/Core.h"

namespace gold::memory
{
	constexpr u32 KB = 1024;
	constexpr u32 MB = 1024 * KB;
	constexpr u32 GB = 1024 * MB;

	struct Memory
	{
		void* data;
		u32 size;
	};

	template<typename T>
	static const std::size_t CalculatePaddingWithHeader(const u64  baseAddress, const u64 alignment) 
	{
		auto calculatePadding = [](const u64  baseAddress, const u64 alignment)
		{
			if (alignment == 0)
			{
				return 0;
			}

			const u64 multiplier = (baseAddress / alignment) + 1;
			const u64  alignedAddress = multiplier * alignment;
			const u64  padding = alignedAddress - baseAddress;
			return padding;
		};

		if (alignment == 0)
		{
			return 0;
		}

		const u64 headerSize = sizeof(T);

		u64 padding = calculatePadding(baseAddress, alignment);
		u64 neededSpace = headerSize;

		if (padding < neededSpace) 
		{
			// Header does not fit - Calculate next aligned address that header fits
			neededSpace -= padding;

			// How many alignments I need to fit the header
			if (neededSpace % alignment > 0) 
			{
				padding += alignment * (1 + (neededSpace / alignment));
			}
			else 
			{
				padding += alignment * (neededSpace / alignment);
			}
		}

		return padding;
	}
}
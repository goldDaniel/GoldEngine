#include "Allocator.h"

using namespace gold;

AllocatorStats Allocator::GetStats() const
{
	return
	{
		mAllocatorSize,
		mUsed,
		mPeak,
	};
}
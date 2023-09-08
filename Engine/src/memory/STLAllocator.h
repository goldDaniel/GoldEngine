#pragma once 

#include "core/Core.h"
#include "Allocator.h"

namespace gold
{
	template<typename T>
	class STLAdapter
	{
	private:
		gold::Allocator& mAllocator;
	public:

		typedef T value_type;

		STLAdapter() = delete;
		
		STLAdapter(gold::Allocator& allocator)
			: mAllocator(allocator)
		{	
		}

		[[nodiscard]] constexpr T* allocate(u64 num)
		{
			return reinterpret_cast<T*>(mAllocator.Allocate(num * sizeof(T), alignof(T)));
		}

		constexpr void deallocate(T* p, std::size_t num) noexcept
		{
			UNUSED_VAR(num);
			mAllocator.Free(p);
		}


		u64 MaxAllocationSize() const noexcept
		{
			return mAllocator.mAllocatorSize;
		}

		bool operator==(const STLAdapter<T>& rhs) const noexcept
		{
			return mAllocator == rhs.mAllocator;
		}

		bool operator!=(const STLAdapter<T>& rhs) const noexcept
		{
			return !(*this == rhs);
		}

	};
}
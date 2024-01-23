#pragma once

#include <core/core.h>
#include "memory/Utils.h"

namespace gold
{

	class Allocator
	{
	public:

		Allocator(void* mem, u64 size);

		virtual ~Allocator();

		virtual void* Allocate(u64 size, u8 alignment = 4) = 0;
		virtual void Deallocate(void* p) = 0;

		void* GetStart() const { return mMemory; }
		u64 GetSize() const { return mSize; }
		u64 GetUsedMemory() const { return mUsed; }
		u64 GetNumAllocations() const { return mNumAllocations; }

		// use if Allocator owns the memory block
		void Free();

	protected:

		void* mMemory;
		u64 mSize;
		u64 mUsed;
		u64 mNumAllocations;
	};

	namespace allocator
	{
		inline void* add(void* p, size_t val)
		{
			return (void*)(reinterpret_cast<u64*>(p) + val);
		}

		inline const void* add(const void* p, size_t val)
		{
			return (const void*)(reinterpret_cast<const u64*>(p) + val);
		}

		inline void* subtract(void* p, size_t val)
		{
			return (void*)(reinterpret_cast<u64*>(p) - val);
		}

		inline const void* subtract(const void* p, size_t val)
		{
			return (const void*)(reinterpret_cast<const u64*>(p) - val);
		}

		inline void* alignForward(const void* address, u8 alignment)
		{
			u64 addressVal = (u64)address;
			addressVal += (alignment - 1);

			u8 alignmentMask = static_cast<u8>(~(alignment - 1));
			
			return (void*)(addressVal & alignmentMask);
		}

		inline u8 alignForwardAdjustment(const void* address, u8 alignment)
		{
			u64 addressVal = (u64)address;
			u64 allignmentMask = (u64)((u8*)((u64)alignment - 1));

			u8 adjustment = alignment - static_cast<u8>(addressVal & allignmentMask);

			if (adjustment == alignment)
			{
				return 0;
			}

			return adjustment;
		}
		
		inline u8 alignForwardAdjustmentWithHeader(const void* address, u8 alignment, u8 headerSize)
		{
			u8 adjustment = alignForwardAdjustment(address, alignment);
			u8 neededSpace = headerSize;

			if (adjustment < neededSpace)
			{
				neededSpace -= adjustment;

				//Increase adjustment to fit header 
				adjustment += alignment * (neededSpace / alignment);

				if (neededSpace % alignment > 0)
				{
					adjustment += alignment;
				}
			}

			return adjustment;
		}

		template <class T> 
		T* allocateNew(gold::Allocator& allocator)
		{
			return new (allocator.allocate(sizeof(T), alignof(T))) T;
		}

		template <class T> 
		T* allocateNew(gold::Allocator& allocator, const T& t)
		{
			return new (allocator.allocate(sizeof(T), alignof(T))) T(t);
		}

		template <class T> 
		void deallocateDelete(gold::Allocator& allocator, T& object)
		{
			object.~T();
			allocator.deallocate(&object);
		}

		template <class T> 
		T* allocateArray(gold::Allocator& allocator, u64 length)
		{
			DEBUG_ASSERT(length != 0, "Attempting to allocate size 0!");

			u8 headerSize = sizeof(u64) / sizeof(T);

			if (sizeof(u64) % sizeof(T) > 0)
			{
				headerSize += 1;
			}

			T* p = ((T*)allocator.allocate(sizeof(T) * (length + headerSize), alignof(T))) + headerSize;

			*(((u64*)p) - 1) = length;

			for (u64 i = 0; i < length; i++)
			{
				new (&p) T;
			}
				
			return p;
		}

		template <class T> 
		void deallocateArray(gold::Allocator& allocator, T* array)
		{
			DEBUG_ASSERT(array != nullptr, "Attempting to deallocate null!");

			u64 length = *(((u64*)array) - 1);

			for (u64 i = 0; i < length; i++)
			{
				array.~T();
			}

			//Calculate how much extra memory was allocated to store the length before the array 
			u8 headerSize = sizeof(u64) / sizeof(T);
			if (sizeof(u64) % sizeof(T) > 0)
			{
				headerSize += 1;
			}
				
			allocator.deallocate(array - headerSize);
		}
	};
}

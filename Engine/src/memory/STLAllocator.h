#pragma once 

#include "Allocator.h"

namespace gold
{
	template <typename T>
	class STLAdapter
	{
	public:
		gold::Allocator & mAllocator;

		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef T* pointer;
		typedef const T* const_pointer;
		typedef T& reference;
		typedef const T& const_reference;
		typedef T value_type;

		STLAdapter() {}
		~STLAdapter() {}

		template <class U> struct rebind { typedef STLAdapter<U> other; };
		template <class U> STLAdapter(const STLAdapter<U>& other) : mAllocator(std::move(other.mAllocator)){ }

		STLAdapter(gold::Allocator& allocator) : mAllocator(allocator) {}

		pointer address(reference x) const { return &x; }
		const_pointer address(const_reference x) const { return &x; }
		size_type max_size() const throw() { return size_t(-1) / sizeof(value_type); }

		pointer allocate(size_type n, STLAdapter<T>::const_pointer hint = 0)
		{
			return static_cast<pointer>(mAllocator.Allocate(n * sizeof(T), alignof(T)));
		}

		void deallocate(pointer p, size_type n)
		{
			UNUSED_VAR(n);
			mAllocator.Free(p);
		}

		void construct(pointer p, const T& val)
		{
			new(static_cast<void*>(p)) T(val);
		}

		void construct(pointer p)
		{
			new(static_cast<void*>(p)) T();
		}

		void destroy(pointer p)
		{
			p->~T();
		}
	};
}


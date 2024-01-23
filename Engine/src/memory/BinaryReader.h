#include "core/Core.h"

namespace gold
{
	class BinaryReader
	{
	private:
		u8* mMemory;
		u64 const mSize;
		u64 mOffset;

	public:
		BinaryReader(u8* memory, u64 size)
			: mMemory(memory)
			, mSize(size)
			, mOffset(0)
		{

		}

		bool HasData() const
		{
			return mSize > 0;
		}

		bool CanReadData() const
		{
			return mOffset < (mSize - 1);
		}

		void Reset()
		{
			mOffset = 0;
		}

		u64 GetSize() const
		{
			return mSize;
		}

		template<typename T>
		const T Read()
		{
			T result{};
			u64 size = sizeof(T);
			DEBUG_ASSERT(size + mOffset <= mSize, "Data overflow!");

			void* address = mMemory + mOffset;
			memcpy(&result, address, size);

			mOffset += size;

			return result;
		}

		const u8* GetData() const
		{
			return mMemory;
		}

		void Read(u8* data, u64 size)
		{
			DEBUG_ASSERT(size + mOffset <= mSize, "Data overflow!");

			void* address = mMemory + mOffset;
			memcpy(data, address, size);

			mOffset += size;
		}

		void Skip(u64 size)
		{
			DEBUG_ASSERT(size + mOffset <= mSize, "Data overflow!");
			mOffset += size;
		}
	};
}

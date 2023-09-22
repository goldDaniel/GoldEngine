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

		void Reset()
		{
			mOffset = 0;
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
		}

		void Read(u8* data, u64 size)
		{
			DEBUG_ASSERT(size + mOffset <= mSize, "Data overflow!");

			void* address = mMemory + mOffset;
			memcpy(data, address, size);

			mOffset += size;
		}
	};
}

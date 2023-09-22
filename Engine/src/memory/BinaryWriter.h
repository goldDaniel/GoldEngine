namespace gold
{
	class BinaryWriter
	{
	private:
		u8* mMemory;
		u64 const mSize;
		u64 mOffset;

	public:
		BinaryWriter(u8* memory, u64 size)
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
		void Write(const T& data)
		{
			u64 size = sizeof(T);
			DEBUG_ASSERT(size + mOffset <= mSize, "Data overflow!");

			void* address = mMemory + mOffset;
			memcpy(address, &data, size);

			mOffset += size;
		}

		void Write(const void* data, u64 size)
		{
			DEBUG_ASSERT(size + mOffset <= mSize, "Data overflow!");

			void* address = mMemory + mOffset;
			memcpy(address, data, size);

			mOffset += size;
		}
	};
}
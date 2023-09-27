#pragma once 

#include <array>
#include <mutex>

namespace gold
{
	template<typename T>
	class DoubleBuffered
	{
	private:
		int mIndex = 0;
		std::array<T, 2> mBufffers;
		mutable std::mutex mMutex;

	public:
		DoubleBuffered() = default;

		void Init(std::function<T()> initializer)
		{
			mBufffers[0] = initializer();
			mBufffers[1] = initializer();
		}

		T& Get()			 { return mBufffers[(mIndex + 1) % 2]; }
		const T& Get() const { return mBufffers[(mIndex + 1) % 2]; }

		void Swap()
		{
			std::unique_lock<std::mutex> lock(mMutex);
			mIndex ^= 1;
		}
	};
}
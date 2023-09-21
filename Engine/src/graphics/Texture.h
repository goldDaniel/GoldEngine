#pragma once

#include <cstdint>
#include <string>

namespace graphics
{
	class Texture2D
	{

	public:
		Texture2D() = default;

		Texture2D(const std::string& filepath);

		~Texture2D();

		int GetWidth() const;

		int GetHeight() const;

		uint32_t GetNameHash() const;

		const void* GetData() const;

		enum class TextureFormat GetFormat() const;

		uint32_t GetDataSize() const;

	private:

		uint32_t mNameHash = 0;

		bool mCompressedLoad = false;

		int mWidth = 0;
		int mHeight = 0;

		int mChannels = 0;

		void* mData = 0;
	};
}


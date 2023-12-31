#pragma once

#include "core/Core.h"
#include "RenderTypes.h"

namespace graphics
{
	class Texture2D
	{

	public:
		Texture2D() = default;

		Texture2D(const std::string& filepath);

		~Texture2D();

		u16 GetWidth() const;

		u16 GetHeight() const;

		u32 GetNameHash() const;

		const void* GetData() const;

		graphics::TextureFormat GetFormat() const;

		u32 GetDataSize() const;

	private:

		u32 mNameHash = 0;

		bool mCompressedLoad = false;

		u16 mWidth = 0;
		u16 mHeight = 0;

		u16 mChannels = 0;

		void* mData = 0;
	};
}

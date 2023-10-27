#include "Texture.h"

#include "core/Util.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


using namespace graphics;

Texture2D::Texture2D(const std::string& filepath)
{
	mNameHash = util::Hash(filepath.c_str(), filepath.size());

	std::string extension = filepath.substr(filepath.find_last_of('.'), filepath.size());

	if (extension == ".dds")
	{
		DEBUG_ASSERT(false, "DDS currently unsupported!");
		// TODO (danielg: DDS support here
		
		//dds_image* image = dds_load_from_file(filepath.c_str());
		//
		//mCompressedLoad = true;
		//
		//mWidth = image->header.width;
		//mHeight = image->header.height;
		//mChannels = 4;

		//// we don't care about the image struct self, but we care about the data
		//mData = image->pixels;
		//free(image);
	}
	else
	{
		mData = stbi_load(filepath.c_str(), &mWidth, &mHeight, &mChannels, STBI_default);
	}
	
}

Texture2D::~Texture2D()
{
	// this is probably not needed?
	if (mCompressedLoad)
	{
		free(mData);
	}
	else
	{
		stbi_image_free(mData);
	}
	
}

u16 Texture2D::GetWidth() const
{
	return mWidth;
}

u16 Texture2D::GetHeight() const
{
	return mHeight;
}

u32 Texture2D::GetNameHash() const
{
	return mNameHash;
}

const void* Texture2D::GetData() const
{
	return mData;
}

u32 Texture2D::GetDataSize() const
{
	return sizeof(uint8_t) * mChannels * mWidth * mHeight;
}

TextureFormat Texture2D::GetFormat() const
{
	switch(mChannels)
	{
	case 1: return TextureFormat::R_U8;
	case 3: return TextureFormat::RGB_U8;
	case 4: return TextureFormat::RGBA_U8;
	}
	
	DEBUG_ASSERT(false, "Unsupported format!");
	return TextureFormat::INVALID;
}

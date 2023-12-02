#include "RenderTypes.h"

#include "Texture.h"

graphics::TextureDescription2D::TextureDescription2D(const Texture2D& tex, bool mipmaps)
{
	mNameHash = tex.GetNameHash();
	mData = tex.GetData();
	mDataSize = tex.GetDataSize();
	mWidth = tex.GetWidth();
	mHeight = tex.GetHeight();
	mFormat = tex.GetFormat();
	mMipmaps = mipmaps;
}

graphics::TextureDescription3D::TextureDescription3D(const std::vector<Texture2D>& data, bool mipmaps)
{
	mWidth = data[0].GetWidth();
	mHeight = data[0].GetHeight();
	mDepth = static_cast<u32>(data.size());

	mDataSize = data[0].GetDataSize();
	mFormat = data[0].GetFormat();
	mMipmaps = mipmaps;

	for (u64 i = 0; i < data.size(); ++i)
	{
		DEBUG_ASSERT(mDataSize == data[i].GetDataSize(), "");
		DEBUG_ASSERT(mWidth == data[i].GetWidth(), "");
		DEBUG_ASSERT(mHeight == data[i].GetHeight(), "");
		DEBUG_ASSERT(mFormat == data[i].GetFormat(), "");

		mData[i] = data[i].GetData();
	}
}

graphics::CubemapDescription::CubemapDescription(const std::unordered_map<CubemapFace, Texture2D&>& data)
{
	DEBUG_ASSERT(data.size() == static_cast<u32>(CubemapFace::COUNT), "Incorrect number of cubemap faces!");

	mDataSize = data.at(CubemapFace::POSITIVE_X).GetDataSize();
	mWidth = data.at(CubemapFace::POSITIVE_X).GetWidth();
	mHeight = data.at(CubemapFace::POSITIVE_X).GetHeight();
	mFormat = data.at(CubemapFace::POSITIVE_X).GetFormat();

	for (const auto& iter : data)
	{
		DEBUG_ASSERT(mDataSize == iter.second.GetDataSize(), "");
		DEBUG_ASSERT(mWidth == iter.second.GetWidth(), "");
		DEBUG_ASSERT(mHeight == iter.second.GetHeight(), "");
		DEBUG_ASSERT(mFormat == iter.second.GetFormat(), "");

		mData[iter.first] = iter.second.GetData();
	}
}

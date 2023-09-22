#pragma once 

#include "core/Core.h"
#include "core/Util.h"
#include "Texture.h"

#define G_RENDER_HANDLE(name) \
	namespace graphics { \
		struct name { \
			uint32_t idx; \
			bool operator==(const name &other) const { return idx == other.idx; } \
		}; \
	} \
	template <> struct std::hash<graphics::##name> {  \
		uint64_t operator()(const graphics::##name& data) const noexcept { \
			return std::hash<uint32_t>{}(data.idx); \
		} \
	};

G_RENDER_HANDLE(VertexBufferHandle);
G_RENDER_HANDLE(IndexBufferHandle);
G_RENDER_HANDLE(UniformBufferHandle);
G_RENDER_HANDLE(ShaderBufferHandle);
G_RENDER_HANDLE(TextureHandle);
G_RENDER_HANDLE(MeshHandle);

#undef G_RENDER_HANDLE

namespace graphics
{
	enum class OutputSlot
	{
		Color0 = 0, // albedo
		Color1,		// normal
		Color2,		// coefficients
		Color3,		// emissive
		Depth,

		Count
	};

	enum class CullFace
	{
		BACK,
		FRONT,
		DISABLED,
	};

	enum class DepthFunction
	{
		LESS,
		LESS_EQUAL,
		EQUAL,
		ALWAYS,
		DISABLED,
	};

	enum class BlendFunction
	{
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		ONE,
		ZERO,
	};

	enum class BufferUsage
	{
		STATIC,
		DYNAMIC,
		STREAM
	};

	enum class PrimitiveType
	{
		TRIANGLES,
		TRIANGLE_STRIP,
		POINTS,
		LINES
	};

	enum class TextureFormat
	{
		INVALID,

		R_U8,
		R_U8NORM,
		R_U16,
		R_FLOAT,

		RGB_U8,
		RGB_U8_SRGB,
		RGB_HALF,
		RGB_FLOAT,

		RGBA_U8,
		RGBA_U8_SRGB,
		RGBA_HALF,
		RGBA_FLOAT,

		DEPTH,
	};

	enum class CubemapFace
	{
		POSITIVE_X = 0,
		NEGATIVE_X,
		POSITIVE_Y,
		NEGATIVE_Y,
		POSITIVE_Z,
		NEGATIVE_Z,

		COUNT,
	};

	enum class TextureWrap
	{
		REPEAT,
		CLAMP,
		MIRROR,
		BORDER
	};

	enum class TextureFilter
	{
		LINEAR,
		POINT,
	};

	enum class FramebufferAttachment
	{
		COLOR0,
		COLOR1,
		COLOR2,
		COLOR3,
		DEPTH,
	};

	enum class VertexFormat
	{
		U8x3,
		U8x4,

		U16x4,

		HALFx2,
		HALFx3,
		HALFx4,

		FLOAT,
		FLOATx2,
		FLOATx3,
		FLOATx4,
	};

	enum class IndexFormat
	{
		U16,
		U32
	};

	enum class ClearColor { YES, NO };
	enum class ClearDepth { YES, NO };

	struct Mesh
	{
		uint32_t  mID{};
		
		uint32_t mVertexCount{};
		uint32_t mIndexCount{};

		uint32_t mIndexStart{};

		PrimitiveType mPrimitiveType{};

		VertexBufferHandle mPositions{};
		VertexBufferHandle mNormals{};
		VertexBufferHandle mTexCoords0{};
		VertexBufferHandle mTexCoords1{};
		VertexBufferHandle mColors{};
		VertexBufferHandle mJoints{};
		VertexBufferHandle mWeights{};

		IndexBufferHandle mIndices{};
		IndexFormat mIndexFormat{};
	};

	struct MeshDescription
	{
		// separate vertex buffers
		struct Handles
		{
			VertexBufferHandle mPositions{};
			VertexBufferHandle mNormals{};
			VertexBufferHandle mTexCoords0{};
			VertexBufferHandle mTexCoords1{};
			VertexBufferHandle mColors{};
			VertexBufferHandle mJoints{};
			VertexBufferHandle mWeights{};
		};

		struct Offsets
		{
			uint32_t mPositionOffset = std::numeric_limits<uint32_t>::max();
			uint32_t mNormalsOffset = std::numeric_limits<uint32_t>::max();
			uint32_t mTexCoord0Offset = std::numeric_limits<uint32_t>::max();
			uint32_t mTexCoord1Offset = std::numeric_limits<uint32_t>::max();
			uint32_t mColorsOffset = std::numeric_limits<uint32_t>::max();
			uint32_t mJointsOffset = std::numeric_limits<uint32_t>::max();
			uint32_t mWeightsOffset = std::numeric_limits<uint32_t>::max();
		};


		// for interlaced vertex buffers
		VertexBufferHandle mInterlacedBuffer{};
		uint32_t mStride = 0;

		union
		{
			Handles handles{ 0 };
			Offsets offsets;
		};

		IndexBufferHandle mIndices{};
		uint32_t mVertexCount = 0;
		uint32_t mIndexCount = 0;
		uint32_t mIndexStart = 0;

		PrimitiveType mPrimitiveType = PrimitiveType::TRIANGLES;

		VertexFormat mPositionFormat = VertexFormat::FLOATx3;
		VertexFormat mNormalsFormat = VertexFormat::FLOATx3;
		VertexFormat mTexCoord0Format = VertexFormat::FLOATx2;
		VertexFormat mTexCoord1Format = VertexFormat::FLOATx2;
		VertexFormat mColorsFormat = VertexFormat::FLOATx3;
		VertexFormat mJointsFormat = VertexFormat::U16x4;
		VertexFormat mWeightsFormat = VertexFormat::FLOATx4;

		IndexFormat mIndicesFormat = IndexFormat::U32;

		MeshDescription() {}
	};


	struct TextureDescription2D
	{
		uint32_t mNameHash = 0;

		uint32_t mWidth = 0;
		uint32_t mHeight = 0;

		const void* mData = nullptr;
		uint32_t mDataSize = 0;

		TextureFormat mFormat = TextureFormat::INVALID;
		TextureWrap mWrap = TextureWrap::REPEAT;
		TextureFilter mFilter = TextureFilter::LINEAR;

		bool mMipmaps = false;

		glm::vec4 mBorderColor{ 0 };

		TextureDescription2D() = default;
		TextureDescription2D(const Texture2D& tex, bool mipmaps)
		{
			mNameHash = tex.GetNameHash();
			mData = tex.GetData();
			mDataSize = tex.GetDataSize();
			mWidth = tex.GetWidth();
			mHeight = tex.GetHeight();
			mFormat = tex.GetFormat();
			mMipmaps = mipmaps;
		}
	};

	struct TextureDescription3D
	{
		uint32_t mWidth = 0;
		uint32_t mHeight = 0;
		uint32_t mDepth = 0;

		//NOTE (danielg): all slices must have same dataSize, width, and height
		std::vector<const void*> mData;
		uint32_t mDataSize = 0;

		TextureFormat mFormat = TextureFormat::INVALID;
		TextureWrap mWrap = TextureWrap::REPEAT;
		TextureFilter mFilter = TextureFilter::LINEAR;

		bool mMipmaps = false;

		glm::vec4 mBorderColor{ 0 };

		TextureDescription3D() = default;
		TextureDescription3D(const std::vector<Texture2D>& data, bool mipmaps)
		{
			mWidth = data[0].GetWidth();
			mHeight = data[0].GetHeight();
			mDepth = static_cast<uint32_t>(data.size());

			mDataSize = data[0].GetDataSize();
			mFormat = data[0].GetFormat();


			for (size_t i = 0; i < data.size(); ++i)
			{
				DEBUG_ASSERT(mDataSize == data[i].GetDataSize(), "");
				DEBUG_ASSERT(mWidth == data[i].GetWidth(), "");
				DEBUG_ASSERT(mHeight == data[i].GetHeight(), "");
				DEBUG_ASSERT(mFormat == data[i].GetFormat(), "");

				mData[i] = data[i].GetData();
			}
		}
	};

	struct CubemapDescription
	{
		//NOTE (danielg): all faces must have same dataSize, width, and height
		std::unordered_map<CubemapFace, const void*> mData;
		uint32_t mDataSize = 0;

		uint32_t mWidth = 0;
		uint32_t mHeight = 0;

		TextureFormat mFormat = TextureFormat::INVALID;
		TextureWrap mWrap = TextureWrap::CLAMP;
		TextureFilter mFilter = TextureFilter::LINEAR;

		CubemapDescription() = default;
		CubemapDescription(const std::unordered_map<CubemapFace, Texture2D&>& data)
		{
			DEBUG_ASSERT(data.size() == static_cast<uint32_t>(CubemapFace::COUNT), "Incorrect number of cubemap faces!");

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
	};



	struct Shader
	{
		uint32_t mHandle = 0;

		// tessellation params
		bool mTesselation = false;

		std::array<uint32_t, 12>  mUniformBlocks; // 12 is the GL defined minimum allowed
		std::array<uint32_t, 8>  mStorageBlocks; // 8 is the GL defined minimum allowed 
		std::array<uint32_t, 16>  mTextures;
		std::array<uint32_t, 16>  mImages;
	};

	struct UniformBuffer
	{
		UniformBufferHandle mID{ 0 };
		uint32_t mSize{ 0 };

		uint8_t* Map();
		void Unmap();
	};

	struct StorageBuffer
	{
		ShaderBufferHandle mID{ 0 };
		uint32_t mSize{ 0 };

		uint8_t* Map();
		void Unmap();
	};

	struct FrameBuffer
	{
		uint32_t mHandle = 0;
		std::array<TextureHandle, static_cast<size_t>(OutputSlot::Count)> mTextures;

		uint32_t mWidth = 0;
		uint32_t mHeight = 0;
	};

	struct FrameBufferDescription
	{
		struct FrameBufferTexture
		{
			TextureDescription2D mDescription{};
			FramebufferAttachment mAttachment{};
		};

		std::array<FrameBufferTexture, static_cast<size_t>(OutputSlot::Count)> mTextures;
	};

	struct RenderPass
	{
		const char* mName = nullptr;

		FrameBuffer mTarget;

		bool mClearColor = false;
		glm::vec4 mColor{ 0,0,0,1 };

		bool mClearDepth = false;
		float mDepth = 1.0f;
	};

	struct RenderState
	{
		struct Viewport
		{
			int x{};
			int y{};
			int width{};
			int height{};

			bool operator== (const Viewport& rhs) const
			{
				return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
			}

			bool operator!= (const Viewport& rhs) const
			{
				return !(*this == rhs);
			}
		};

		struct UniformBlock
		{
			uint32_t mNameHash{};
			UniformBufferHandle mBinding{};
		};

		struct StorageBlock
		{
			uint32_t mNameHash{};
			ShaderBufferHandle mBinding{};
		};

		struct Texture
		{
			uint32_t mNameHash{};
			TextureHandle mHandle{};
		};

		struct Image
		{
			uint32_t mNameHash{};
			TextureHandle mHandle{};
			bool read{};
			bool write{};
		};

		std::array<UniformBlock, 12> mUniformBlocks{};
		size_t mNumUniformBlocks = 0;

		std::array<StorageBlock, 8> mStorageBlocks{};
		size_t mNumStorageBlocks = 0;

		std::array<Texture, 16> mTextures{};
		size_t mNumTextures = 0;

		std::array<Image, 16> mImages{};
		size_t mNumImages = 0;

		uint8_t mRenderPass = std::numeric_limits<uint8_t>::max();
		const Shader* mShader = nullptr;

		Viewport viewport{ 0,0,0,0 };

		DepthFunction mDepthFunc = DepthFunction::LESS;
		BlendFunction mSrcBlendFunc = BlendFunction::SRC_ALPHA;
		BlendFunction mDstBlendFunc = BlendFunction::ONE_MINUS_SRC_ALPHA;
		CullFace mCullFace = CullFace::BACK;

		bool mDepthWriteEnabled = true;
		bool mColorWriteEnabled = true;
		bool mAlphaBlendEnabled = true;

		bool mWireFrame = false;


		void SetUniformBlock(const std::string& name, UniformBufferHandle binding)
		{
			// These constants much match
			DEBUG_ASSERT(mUniformBlocks.size() == mShader->mUniformBlocks.size(), "Uniform block constant size mismatch!");


			u64 nameHash = util::Hash(name.c_str(), name.size());

			// update uniform block if it exists
			for (size_t i = 0; i < mNumUniformBlocks; ++i)
			{
				if (mUniformBlocks[i].mNameHash == nameHash)
				{
					mUniformBlocks[i].mBinding = binding;
					return;
				}
			}

			// insert new uniform block
			DEBUG_ASSERT(mNumUniformBlocks < mUniformBlocks.size(), "Uniform block overflow!");

			mUniformBlocks[mNumUniformBlocks].mNameHash = nameHash;
			mUniformBlocks[mNumUniformBlocks].mBinding = binding;
			mNumUniformBlocks++;
		}

		void SetStorageBlock(const std::string& name, ShaderBufferHandle binding)
		{
			// These constants much match
			DEBUG_ASSERT(mStorageBlocks.size() == mShader->mStorageBlocks.size(), "Storage block constant size mismatch!");

			u64 nameHash = util::Hash(name.c_str(), name.size());

			// update uniform block if it exists
			for (size_t i = 0; i < mNumStorageBlocks; ++i)
			{
				if (mStorageBlocks[i].mNameHash == nameHash)
				{
					mStorageBlocks[i].mBinding = binding;
					return;
				}
			}

			// insert new uniform block
			DEBUG_ASSERT(mNumStorageBlocks < mStorageBlocks.size(), "Storage block overflow!");

			mStorageBlocks[mNumStorageBlocks].mNameHash = nameHash;
			mStorageBlocks[mNumStorageBlocks].mBinding = binding;
			mNumStorageBlocks++;
		}


		void SetTexture(const std::string& name, TextureHandle texture)
		{
			// These constants much match
			DEBUG_ASSERT(mTextures.size() == mShader->mTextures.size(), "Texture slot size mismatch!");

			u64 nameHash = util::Hash(name.c_str(), name.size());

			// update texture if it exists
			for (size_t i = 0; i < mNumTextures; ++i)
			{
				if (mTextures[i].mNameHash == nameHash)
				{
					mTextures[i].mHandle = texture;
					return;
				}
			}

			// insert new texture
			DEBUG_ASSERT(mNumTextures < mTextures.size(), "Texture slot overflow!");

			mTextures[mNumTextures].mNameHash = nameHash;
			mTextures[mNumTextures].mHandle = texture;
			mNumTextures++;
		}

		void SetImage(const std::string& name, TextureHandle texture, bool read = true, bool write = true)
		{
			// These constants much match
			DEBUG_ASSERT(mImages.size() == mShader->mImages.size(), "Image slot size mismatch!");

			u64 nameHash = util::Hash(name.c_str(), name.size());

			// update texture if it exists
			for (size_t i = 0; i < mNumImages; ++i)
			{
				if (mImages[i].mNameHash == nameHash)
				{
					mImages[i].mHandle = texture;
					return;
				}
			}

			// insert new texture
			DEBUG_ASSERT(mNumImages < mImages.size(), "Image slot overflow!");

			mImages[mNumImages].mNameHash = nameHash;
			mImages[mNumImages].mHandle = texture;
			mImages[mNumImages].read = read;
			mImages[mNumImages].write = write;
			mNumImages++;
		}
	};
}
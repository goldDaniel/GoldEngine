#pragma once 

#include "core/Core.h"
#include "core/Util.h"

#define G_RENDER_HANDLE(name) \
	namespace graphics { \
		struct name { \
			u32 idx; \
			void operator=(const name &rhs) { idx = rhs.idx; } \
			bool operator==(const name &other) const { return idx == other.idx; } \
		}; \
	} \
	template <> struct std::hash<graphics::##name> {  \
		u64 operator()(const graphics::##name& data) const noexcept { \
			return std::hash<u32>{}(data.idx); \
		} \
	};

G_RENDER_HANDLE(VertexBufferHandle);
G_RENDER_HANDLE(IndexBufferHandle);
G_RENDER_HANDLE(UniformBufferHandle);
G_RENDER_HANDLE(ShaderBufferHandle);
G_RENDER_HANDLE(TextureHandle);
G_RENDER_HANDLE(MeshHandle);
G_RENDER_HANDLE(ShaderHandle);

#undef G_RENDER_HANDLE

namespace graphics
{
	enum class OutputSlot : u8
	{
		Color0 = 0, // albedo
		Color1,		// normal
		Color2,		// coefficients
		Color3,		// emissive
		Depth,

		Count
	};

	enum class CullFace : u8
	{
		BACK,
		FRONT,
		DISABLED,
	};

	enum class DepthFunction : u8
	{
		LESS,
		LESS_EQUAL,
		EQUAL,
		ALWAYS,
		DISABLED,
	};

	enum class BlendFunction : u8
	{
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		ONE,
		ZERO,
	};

	enum class BufferUsage : u8
	{
		STATIC,
		DYNAMIC,
		STREAM
	};

	enum class PrimitiveType : u8
	{
		TRIANGLES,
		TRIANGLE_STRIP,
		POINTS,
		LINES
	};

	enum class TextureFormat : u8
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

	enum class CubemapFace : u8
	{
		POSITIVE_X = 0,
		NEGATIVE_X,
		POSITIVE_Y,
		NEGATIVE_Y,
		POSITIVE_Z,
		NEGATIVE_Z,

		COUNT,
	};

	enum class TextureWrap : u8
	{
		REPEAT,
		CLAMP,
		MIRROR,
		BORDER
	};

	enum class TextureFilter : u8
	{
		LINEAR,
		POINT,
	};

	enum class FramebufferAttachment : u8
	{
		COLOR0,
		COLOR1,
		COLOR2,
		COLOR3,
		DEPTH,
	};

	enum class VertexFormat : u8
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

	enum class IndexFormat : u8
	{
		U16,
		U32
	};

	enum class ClearColor : u8 { YES, NO };
	enum class ClearDepth : u8 { YES, NO };

	struct Mesh
	{
		u32  mID{};
		
		u32 mVertexCount{};
		u32 mIndexCount{};

		u32 mIndexStart{};

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
			u32 mPositionOffset = std::numeric_limits<u32>::max();
			u32 mNormalsOffset = std::numeric_limits<u32>::max();
			u32 mTexCoord0Offset = std::numeric_limits<u32>::max();
			u32 mTexCoord1Offset = std::numeric_limits<u32>::max();
			u32 mColorsOffset = std::numeric_limits<u32>::max();
			u32 mJointsOffset = std::numeric_limits<u32>::max();
			u32 mWeightsOffset = std::numeric_limits<u32>::max();
		};


		// for interlaced vertex buffers
		VertexBufferHandle mInterlacedBuffer{};
		u32 mStride = 0;

		union
		{
			Handles handles{ 0 };
			Offsets offsets;
		};

		IndexBufferHandle mIndices{};
		u32 mVertexCount = 0;
		u32 mIndexCount = 0;
		u32 mIndexStart = 0;

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
		u32 mNameHash = 0;

		u32 mWidth = 0;
		u32 mHeight = 0;

		const void* mData = nullptr;
		u32 mDataSize = 0;

		TextureFormat mFormat = TextureFormat::INVALID;
		TextureWrap mWrap = TextureWrap::REPEAT;
		TextureFilter mFilter = TextureFilter::LINEAR;

		bool mMipmaps = false;

		glm::vec4 mBorderColor{ 0 };

		TextureDescription2D() = default;
		TextureDescription2D(const class Texture2D& tex, bool mipmaps);
	};

	struct TextureDescription3D
	{
		u32 mWidth = 0;
		u32 mHeight = 0;
		u32 mDepth = 0;

		//NOTE (danielg): all slices must have same dataSize, width, and height
		std::vector<const void*> mData;
		u32 mDataSize = 0;

		TextureFormat mFormat = TextureFormat::INVALID;
		TextureWrap mWrap = TextureWrap::REPEAT;
		TextureFilter mFilter = TextureFilter::LINEAR;

		bool mMipmaps = false;

		glm::vec4 mBorderColor{ 0 };

		TextureDescription3D() = default;
		TextureDescription3D(const std::vector<Texture2D>& data, bool mipmaps);
	};

	struct CubemapDescription
	{
		//NOTE (danielg): all faces must have same dataSize, width, and height
		std::unordered_map<CubemapFace, const void*> mData;
		u32 mDataSize = 0;

		u32 mWidth = 0;
		u32 mHeight = 0;

		TextureFormat mFormat = TextureFormat::INVALID;
		TextureWrap mWrap = TextureWrap::CLAMP;
		TextureFilter mFilter = TextureFilter::LINEAR;

		CubemapDescription() = default;
		CubemapDescription(const std::unordered_map<CubemapFace, Texture2D&>& data);
	};

	struct Shader
	{
		ShaderHandle mHandle{};

		// tessellation params
		bool mTesselation = false;

		std::array<u32, 12>  mUniformBlocks; // 12 is the GL defined minimum allowed
		std::array<u32, 8>  mStorageBlocks; // 8 is the GL defined minimum allowed 
		std::array<u32, 16>  mTextures;
		std::array<u32, 16>  mImages;
	};

	struct UniformBuffer
	{
		UniformBufferHandle mID{ 0 };
		u32 mSize{ 0 };

		u8* Map();
		void Unmap();
	};

	struct StorageBuffer
	{
		ShaderBufferHandle mID{ 0 };
		u32 mSize{ 0 };

		u8* Map();
		void Unmap();
	};

	struct FrameBuffer
	{
		u32 mHandle = 0;
		std::array<TextureHandle, static_cast<u64>(OutputSlot::Count)> mTextures;

		u32 mWidth = 0;
		u32 mHeight = 0;
	};

	struct FrameBufferDescription
	{
		struct FrameBufferTexture
		{
			TextureDescription2D mDescription{};
			FramebufferAttachment mAttachment{};
		};

		std::array<FrameBufferTexture, static_cast<u64>(OutputSlot::Count)> mTextures;
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
			u32 mNameHash{};
			UniformBufferHandle mBinding{};
		};

		struct StorageBlock
		{
			u32 mNameHash{};
			ShaderBufferHandle mBinding{};
		};

		struct Texture
		{
			u32 mNameHash{};
			TextureHandle mHandle{};
		};

		struct Image
		{
			u32 mNameHash{};
			TextureHandle mHandle{};
			bool read{};
			bool write{};
		};

		std::array<UniformBlock, 12> mUniformBlocks{};
		u64 mNumUniformBlocks = 0;

		std::array<StorageBlock, 8> mStorageBlocks{};
		u64 mNumStorageBlocks = 0;

		std::array<Texture, 16> mTextures{};
		u64 mNumTextures = 0;

		std::array<Image, 16> mImages{};
		u64 mNumImages = 0;

		u8 mRenderPass = std::numeric_limits<u8>::max();
		ShaderHandle mShader{};

		Viewport mViewport{ 0,0,0,0 };

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
			u64 nameHash = util::Hash(name.c_str(), name.size());

			// update uniform block if it exists
			for (u64 i = 0; i < mNumUniformBlocks; ++i)
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
			u64 nameHash = util::Hash(name.c_str(), name.size());

			// update uniform block if it exists
			for (u64 i = 0; i < mNumStorageBlocks; ++i)
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
			u64 nameHash = util::Hash(name.c_str(), name.size());

			// update texture if it exists
			for (u64 i = 0; i < mNumTextures; ++i)
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
			u64 nameHash = util::Hash(name.c_str(), name.size());

			// update texture if it exists
			for (u64 i = 0; i < mNumImages; ++i)
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
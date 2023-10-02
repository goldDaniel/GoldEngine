#include "FrameEncoder.h"

#include "RenderCommands.h"

using namespace graphics;
using namespace gold;

static void WriteCreateTexture2D(const TextureDescription2D& desc, BinaryWriter& writer)
{
	writer.Write(desc.mNameHash);
	writer.Write(desc.mWidth);
	writer.Write(desc.mHeight);
	writer.Write(desc.mDataSize);
	if (desc.mDataSize > 0)
	{
		writer.Write(desc.mData, desc.mDataSize);
	}

	writer.Write(desc.mFormat);
	writer.Write(desc.mWrap);
	writer.Write(desc.mFilter);
	
	writer.Write(desc.mMipmaps);
	writer.Write(desc.mBorderColor);
}

FrameEncoder::FrameEncoder(ClientResources& resources, u64 virtualCommandListSize)
	: mMemory(static_cast<u8*>(malloc(virtualCommandListSize)))
	, mWriter(mMemory, virtualCommandListSize)
	, mResources(resources)
	, mNextPass(0)
{

}

FrameEncoder::~FrameEncoder()
{
	DEBUG_ASSERT(!mRecording, "Destroyed while recording frame!");
	free(mMemory);
}

void FrameEncoder::Begin()
{
	DEBUG_ASSERT(!mRecording, "Must end begin recording before beginning");
	mRecording = true;
	mWriter.Reset();
	mNextPass = 0;
}

void FrameEncoder::End()
{
	DEBUG_ASSERT(mRecording, "Must begin recording before ending");
	mWriter.Write(RenderCommand::END);
	mRecording = false;
}

BinaryReader FrameEncoder::GetReader()
{
	DEBUG_ASSERT(!mRecording, "Cannot read frame if encoding!");
	return mWriter.ToReader();
}

// Render pass ///////////////////////////////////

u8 FrameEncoder::AddRenderPass(const graphics::RenderPass& pass)
{
	DEBUG_ASSERT(mRecording, "");

	mWriter.Write(RenderCommand::AddRenderPass);

	// name
	u32 size = strlen(pass.mName) + 1;
	mWriter.Write(size);
	mWriter.Write(pass.mName, size);

	// framebuffer
	mWriter.Write(pass.mTarget); // client handle

	// clear color/depth
	u8 clearColorBit = 1 << 0;
	u8 clearDepthBit = 1 << 1;

	u8 clearBits =	(pass.mClearColor ? clearColorBit : 0) |
					(pass.mClearDepth ? clearDepthBit : 0);
	mWriter.Write(clearBits);
	
	mWriter.Write(pass.mColor); 
	mWriter.Write(pass.mDepth);

	return mNextPass++;
}

u8 FrameEncoder::AddRenderPass(const char* name, FrameBufferHandle target, ClearColor color, ClearDepth depth)
{
	RenderPass pass{};
	pass.mName = name;
	pass.mTarget = target;
	pass.mClearColor = color == ClearColor::YES;
	pass.mClearDepth = depth == ClearDepth::YES;

	return AddRenderPass(pass);
}

u8 FrameEncoder::AddRenderPass(const char* name, ClearColor color, ClearDepth depth)
{
	FrameBufferHandle fb{};
	return AddRenderPass(name, fb, color, depth);
}

// Index Buffer //////////////////////////////////////////////

IndexBufferHandle FrameEncoder::CreateIndexBuffer(const void* data, u32 size)
{
	DEBUG_ASSERT(mRecording, "");

	IndexBufferHandle clientHandle = mResources.CreateIndexBuffer();

	mWriter.Write(RenderCommand::CreateIndexBuffer);
	mWriter.Write(clientHandle);
	mWriter.Write(size);
	mWriter.Write(data, size);

	return clientHandle;
}

VertexBufferHandle FrameEncoder::CreateVertexBuffer(const void* data, u32 size)
{
	DEBUG_ASSERT(mRecording, "");

	VertexBufferHandle clientHandle = mResources.CreateVertexBuffer();

	mWriter.Write(RenderCommand::CreateVertexBuffer);
	mWriter.Write(clientHandle);
	mWriter.Write(size);
	mWriter.Write(data, size);

	return clientHandle;
}

// Uniform Buffers ////////////////////////////////

UniformBufferHandle FrameEncoder::CreateUniformBuffer(const void* data, u32 size)
{
	DEBUG_ASSERT(mRecording, "");

	UniformBufferHandle clientHandle = mResources.CreateUniformBuffer();

	mWriter.Write(RenderCommand::CreateUniformBuffer);
	mWriter.Write(clientHandle);
	mWriter.Write(size);
	mWriter.Write(data, size);

	return clientHandle;
}

void FrameEncoder::UpdateUniformBuffer(UniformBufferHandle clientHandle, const void* data, u32 size, u32 offset)
{
	DEBUG_ASSERT(mRecording, "");

	mWriter.Write(RenderCommand::UpdateUniformBuffer);
	mWriter.Write(clientHandle);
	mWriter.Write(size);
	mWriter.Write(data, size);
	mWriter.Write(offset);
}

void FrameEncoder::DestroyUniformBuffer(UniformBufferHandle clientHandle)
{
	mWriter.Write(RenderCommand::DestroyUniformBuffer);
	mWriter.Write(clientHandle);
}

// Shader Buffers ///////////////////////////////////

ShaderBufferHandle FrameEncoder::CreateShaderBuffer(const void* data, u32 size)
{
	DEBUG_ASSERT(mRecording, "");

	ShaderBufferHandle clientHandle = mResources.CreateShaderBuffer();

	mWriter.Write(RenderCommand::CreateShaderBuffer);
	mWriter.Write(clientHandle);
	mWriter.Write(size);
	mWriter.Write(data, size);

	return clientHandle;
}

void FrameEncoder::UpdateShaderBuffer(graphics::ShaderBufferHandle clientHandle, const void* data, u32 size, u32 offset)
{
	mWriter.Write(RenderCommand::UpdateShaderBuffer);
	mWriter.Write(clientHandle);
	mWriter.Write(size);
	mWriter.Write(data, size);
	mWriter.Write(offset);
}

ShaderHandle FrameEncoder::CreateShader(const char* vertSrc, const char* fragSrc)
{
	DEBUG_ASSERT(mRecording, "");
	mWriter.Write(RenderCommand::CreateShader);

	ShaderHandle clientHandle = mResources.CreateShader();

	mWriter.Write(clientHandle);

	u32 vertLen = strlen(vertSrc) + 1;
	u32 fragLen = strlen(fragSrc) + 1;

	// vert
	mWriter.Write(vertLen);
	mWriter.Write(vertSrc, vertLen);
	
	// frag
	mWriter.Write(fragLen);
	mWriter.Write(fragSrc, fragLen);

	return clientHandle;
}

MeshHandle FrameEncoder::CreateMesh(const MeshDescription& desc)
{
	DEBUG_ASSERT(mRecording, "");

	mWriter.Write(RenderCommand::CreateMesh);

	MeshHandle clientHandle = mResources.CreateMesh();

	mWriter.Write(clientHandle);

	mWriter.Write(desc.mInterlacedBuffer);
	mWriter.Write(desc.mStride);

	// Interlaced, write out offsets
	if (desc.mInterlacedBuffer.idx)
	{
		mWriter.Write(desc.offsets.mPositionOffset);
		mWriter.Write(desc.offsets.mNormalsOffset);
		mWriter.Write(desc.offsets.mTexCoord0Offset);
		mWriter.Write(desc.offsets.mTexCoord1Offset);
		mWriter.Write(desc.offsets.mColorsOffset);
		mWriter.Write(desc.offsets.mJointsOffset);
		mWriter.Write(desc.offsets.mWeightsOffset);
	}
	else
	{
		// not interlaced, write out handles
		mWriter.Write(desc.handles.mPositions);
		mWriter.Write(desc.handles.mNormals);
		mWriter.Write(desc.handles.mTexCoords0);
		mWriter.Write(desc.handles.mTexCoords1);
		mWriter.Write(desc.handles.mColors);
		mWriter.Write(desc.handles.mJoints);
		mWriter.Write(desc.handles.mWeights);
	}

	mWriter.Write(desc.mIndices);
	mWriter.Write(desc.mVertexCount);
	mWriter.Write(desc.mIndexCount);
	mWriter.Write(desc.mIndexStart);

	mWriter.Write(desc.mPrimitiveType);

	mWriter.Write(desc.mPositionFormat);
	mWriter.Write(desc.mNormalsFormat);
	mWriter.Write(desc.mTexCoord0Format);
	mWriter.Write(desc.mTexCoord1Format);
	mWriter.Write(desc.mColorsFormat);
	mWriter.Write(desc.mJointsFormat);
	mWriter.Write(desc.mWeightsFormat);

	mWriter.Write(desc.mIndicesFormat);

	return clientHandle;
}

graphics::TextureHandle FrameEncoder::CreateTexture2D(const graphics::TextureDescription2D& desc)
{
	mWriter.Write(RenderCommand::CreateTexture2D);

	graphics::TextureHandle clientHandle = mResources.CreateTexture();
	mWriter.Write(clientHandle);

	WriteCreateTexture2D(desc, mWriter);

	return clientHandle;
}

FrameBuffer FrameEncoder::CreateFrameBuffer(const FrameBufferDescription& desc)
{
	mWriter.Write(RenderCommand::CreateFrameBuffer);
	FrameBufferHandle clientHandle = mResources.CreateFrameBuffer();
	mWriter.Write(clientHandle);

	FrameBuffer result{ clientHandle };
	
	constexpr u32 maxAttachments = static_cast<u64>(OutputSlot::Count);
	for (u32 i = 0; i < maxAttachments; ++i)
	{
		if (desc.mTextures[i].mDescription.mFormat != TextureFormat::INVALID)
		{
			result.mTextures[i] = mResources.CreateTexture();
			result.mWidth = desc.mTextures[i].mDescription.mWidth;
			result.mHeight = desc.mTextures[i].mDescription.mHeight;
		}

		mWriter.Write(result.mTextures[i]);
		WriteCreateTexture2D(desc.mTextures[i].mDescription, mWriter);
		mWriter.Write(desc.mTextures[i].mAttachment);
	}

	DEBUG_ASSERT(result.mWidth  > 0, "Invalid framebuffer size!");
	DEBUG_ASSERT(result.mHeight > 0, "Invalid framebuffer size!");
	return result;
}

void FrameEncoder::DrawMesh(const MeshHandle handle, const RenderState& state)
{
	DEBUG_ASSERT(mRecording, "");

	mWriter.Write(RenderCommand::DrawMesh);
	mWriter.Write(handle);
	
	// uniform buffers
	mWriter.Write(state.mNumUniformBlocks);
	for (u8 i = 0; i < state.mNumUniformBlocks; ++i)
	{
		const RenderState::UniformBlock& buffer = state.mUniformBlocks[i];
		mWriter.Write(buffer.mNameHash);
		mWriter.Write(buffer.mBinding);
	}

	// Shader buffers
	mWriter.Write(state.mNumStorageBlocks);
	for (u8 i = 0; i < state.mNumStorageBlocks; ++i)
	{
		const RenderState::StorageBlock& buffer = state.mStorageBlocks[i];
		mWriter.Write(buffer.mNameHash);
		mWriter.Write(buffer.mBinding);
	}

	// Textures
	mWriter.Write(state.mNumTextures);
	for (u8 i = 0; i < state.mNumTextures; ++i)
	{
		const RenderState::Texture& texture = state.mTextures[i];
		mWriter.Write(texture.mNameHash);
		mWriter.Write(texture.mHandle);
	}

	// Images
	mWriter.Write(state.mNumImages);
	for (u8 i = 0; i < state.mNumImages; ++i)
	{
		const RenderState::Image& image = state.mImages[i];
		mWriter.Write(image.mNameHash);
		mWriter.Write(image.mHandle);

		u8 readBit  = image.read  ? 1 << 0 : 0;
		u8 writeBit = image.write ? 1 << 1 : 0;
		u8 readWrite = readBit | writeBit;
		
		mWriter.Write(readWrite);
	}
	
	// Render pass
	mWriter.Write(state.mRenderPass);

	// shader handle
	mWriter.Write(state.mShader);

	// Viewport
	mWriter.Write(state.mViewport.x);
	mWriter.Write(state.mViewport.y);
	mWriter.Write(state.mViewport.width);
	mWriter.Write(state.mViewport.height);

	// Depth Func
	mWriter.Write(state.mDepthFunc);

	// Blend Func
	mWriter.Write(state.mSrcBlendFunc);
	mWriter.Write(state.mDstBlendFunc);

	// CullFace
	mWriter.Write(state.mCullFace);

	// enable/disable booleans
	u8 depthWriteBit = 1 << 0;
	u8 colorWriteBit = 1 << 1;
	u8 alphaBlendBit = 1 << 2;
	u8 wireframeBit  = 1 << 3;
	
	u8 toggles =	(state.mDepthWriteEnabled ? depthWriteBit : 0) |
					(state.mColorWriteEnabled ? colorWriteBit : 0) |
					(state.mAlphaBlendEnabled ? alphaBlendBit : 0) |
					(state.mWireFrame		 ? wireframeBit  : 0);
	
	mWriter.Write(toggles);
}

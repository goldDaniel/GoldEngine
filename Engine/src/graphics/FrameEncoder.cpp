#include "FrameEncoder.h"

#include "RenderCommands.h"

using namespace graphics;
using namespace gold;

FrameEncoder::FrameEncoder(RenderResources& resources)
	: mMemory(static_cast<u8*>(malloc(size)))
	, mWriter(mMemory, size)
	, mResources(resources)
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
}

void FrameEncoder::End()
{
	DEBUG_ASSERT(mRecording, "Must begin recording before ending");
	mWriter.Write(RenderCommand::END);
	mRecording = false;
}

IndexBufferHandle FrameEncoder::CreateIndexBuffer(const void* data, u64 size)
{
	DEBUG_ASSERT(mRecording, "");

	IndexBufferHandle clientHandle = mResources.createIndexBuffer();

	mWriter.Write(RenderCommand::CreateIndexBuffer);
	mWriter.Write(clientHandle);
	mWriter.Write(size);
	mWriter.Write(data, size);

	return clientHandle;
}

VertexBufferHandle FrameEncoder::CreateVertexBuffer(const void* data, u64 size)
{
	DEBUG_ASSERT(mRecording, "");

	VertexBufferHandle clientHandle = mResources.createVertexBuffer();

	mWriter.Write(RenderCommand::CreateVertexBuffer);
	mWriter.Write(clientHandle);
	mWriter.Write(size);
	mWriter.Write(data, size);

	return clientHandle;
}

UniformBufferHandle FrameEncoder::CreateUniformBuffer(const void* data, u64 size)
{
	DEBUG_ASSERT(mRecording, "");

	UniformBufferHandle clientHandle = mResources.CreateUniformBuffer(size);

	mWriter.Write(RenderCommand::CreateUniformBuffer);
	mWriter.Write(clientHandle.idx);
	mWriter.Write(size);
	mWriter.Write(data, size);

	return clientHandle;
}

ShaderBufferHandle FrameEncoder::CreateShaderBuffer(const void* data, u64 size)
{
	DEBUG_ASSERT(mRecording, "");

	ShaderBufferHandle clientHandle = mResources.CreateShaderBuffer(size);

	mWriter.Write(RenderCommand::CreateShaderBuffer);
	mWriter.Write(clientHandle.idx);
	mWriter.Write(size);
	mWriter.Write(data, size);

	return clientHandle;
}

void FrameEncoder::DrawMesh(const MeshHandle handle, const RenderState& state)
{
	DEBUG_ASSERT(mRecording, "");

	mWriter.Write(RenderCommand::DrawMesh);
	mWriter.Write(handle.idx);
	
	// uniform buffers
	mWriter.Write(state.mUniformBlocks);
	for (u8 i = 0; i < state.mNumUniformBlocks; ++i)
	{
		const RenderState::UniformBlock& buffer = state.mUniformBlocks[i];
		mWriter.Write(buffer.mNameHash);
		mWriter.Write(buffer.mBinding.idx);
	}

	// Shader buffers
	mWriter.Write(state.mNumStorageBlocks);
	for (u8 i = 0; i < state.mNumStorageBlocks; ++i)
	{
		const RenderState::StorageBlock& buffer = state.mStorageBlocks[i];
		mWriter.Write(buffer.mNameHash);
		mWriter.Write(buffer.mBinding.idx);
	}

	// Textures
	mWriter.Write(state.mNumTextures);
	for (u8 i = 0; i < state.mNumTextures; ++i)
	{
		const RenderState::Texture& texture = state.mTextures[i];
		mWriter.Write(texture.mNameHash);
		mWriter.Write(texture.mHandle.idx);
	}

	// Images
	mWriter.Write(state.mNumImages);
	for (u8 i = 0; i < state.mNumImages; ++i)
	{
		const RenderState::Image& image = state.mImages[i];
		mWriter.Write(image.mNameHash);
		mWriter.Write(image.mHandle.idx);

		u8 readBit  = image.read  ? 1 << 0 : 0;
		u8 writeBit = image.write ? 1 << 1 : 0;
		u8 readWrite = readBit | writeBit;
		
		mWriter.Write(readWrite);
	}
	
	// Render pass
	mWriter.Write(state.mRenderPass);

	// shader handle
	mWriter.Write(state.mShader.idx);

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
	
	u8 toggles =	state.mDepthWriteEnabled ? depthWriteBit : 0 ||
					state.mColorWriteEnabled ? colorWriteBit : 0 ||
					state.mAlphaBlendEnabled ? alphaBlendBit : 0 ||
					state.mWireFrame		 ? wireframeBit  : 0;
	
	mWriter.Write(toggles);
}

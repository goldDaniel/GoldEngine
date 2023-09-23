#include "FrameEncoder.h"

#include "RenderCommands.h"

using namespace graphics;
using namespace gold;

FrameEncoder::FrameEncoder(ClientResources& resources)
	: mMemory(static_cast<u8*>(malloc(kSize)))
	, mWriter(mMemory, kSize)
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

u8 FrameEncoder::AddRenderPass(const graphics::RenderPass& pass)
{
	DEBUG_ASSERT(mRecording, "");

	mWriter.Write(RenderCommand::AddRenderPass);

	// name
	u64 size = strlen(pass.mName) + 1;
	mWriter.Write(size);
	mWriter.Write(pass.mName, size);

	// framebuffer
	mWriter.Write(pass.mTarget.mHandle); // client handle
	for (u64 i = 0; i < static_cast<u64>(OutputSlot::Count); ++i)
	{
		mWriter.Write(pass.mTarget.mTextures[i]); // client handles
	}
	mWriter.Write(pass.mTarget.mWidth);
	mWriter.Write(pass.mTarget.mHeight);


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

u8 FrameEncoder::AddRenderPass(const char* name, FrameBuffer target, ClearColor color, ClearDepth depth)
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
	FrameBuffer framebufffer{};
	return AddRenderPass(name, framebufffer, color, depth);
}

IndexBufferHandle FrameEncoder::CreateIndexBuffer(const void* data, u64 size)
{
	DEBUG_ASSERT(mRecording, "");

	IndexBufferHandle clientHandle = mResources.CreateIndexBuffer();

	mWriter.Write(RenderCommand::CreateIndexBuffer);
	mWriter.Write(clientHandle);
	mWriter.Write(size);
	mWriter.Write(data, size);

	return clientHandle;
}

VertexBufferHandle FrameEncoder::CreateVertexBuffer(const void* data, u64 size)
{
	DEBUG_ASSERT(mRecording, "");

	VertexBufferHandle clientHandle = mResources.CreateVertexBuffer();

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
	mWriter.Write(clientHandle);
	mWriter.Write(size);
	mWriter.Write(data, size);

	return clientHandle;
}

ShaderBufferHandle FrameEncoder::CreateShaderBuffer(const void* data, u64 size)
{
	DEBUG_ASSERT(mRecording, "");

	ShaderBufferHandle clientHandle = mResources.CreateShaderBuffer(size);

	mWriter.Write(RenderCommand::CreateShaderBuffer);
	mWriter.Write(clientHandle);
	mWriter.Write(size);
	mWriter.Write(data, size);

	return clientHandle;
}

ShaderHandle FrameEncoder::CreateShader(const char* vertSrc, const char* fragSrc)
{
	DEBUG_ASSERT(mRecording, "");
	mWriter.Write(RenderCommand::CreateShader);

	ShaderHandle clientHandle = mResources.CreateShader();

	mWriter.Write(clientHandle);

	u64 vertLen = strlen(vertSrc) + 1;
	u64 fragLen = strlen(fragSrc) + 1;

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
	return {0};
}

void FrameEncoder::DrawMesh(const MeshHandle handle, const RenderState& state)
{
	DEBUG_ASSERT(mRecording, "");

	mWriter.Write(RenderCommand::DrawMesh);
	mWriter.Write(handle);
	
	// uniform buffers
	mWriter.Write(state.mUniformBlocks);
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

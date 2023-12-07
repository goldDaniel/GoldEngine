#include "FrameEncoder.h"

#include "RenderCommands.h"

using namespace graphics;
using namespace gold;

static void WriteCreateTexture2D(const TextureDescription2D& desc, BinaryWriter& writer, LinearAllocator& allocator)
{
	writer.Write(desc.mNameHash);
	writer.Write(desc.mWidth);
	writer.Write(desc.mHeight);
	writer.Write(desc.mDataSize);
	if (desc.mDataSize > 0)
	{
		void* data = allocator.Allocate(desc.mDataSize);
		memcpy(data, desc.mData, desc.mDataSize);
		writer.Write(Memory{data, desc.mDataSize});
	}

	writer.Write(desc.mFormat);
	writer.Write(desc.mWrap);
	writer.Write(desc.mFilter);
	
	writer.Write(desc.mMipmaps);
	writer.Write(desc.mBorderColor);
}

static void WriteRenderState(const RenderState& state, BinaryWriter& writer)
{
	// uniform buffers
	writer.Write(state.mNumUniformBlocks);
	for (u8 i = 0; i < state.mNumUniformBlocks; ++i)
	{
		const RenderState::UniformBlock& buffer = state.mUniformBlocks[i];
		writer.Write(buffer.mNameHash);
		writer.Write(buffer.mHandle);
	}

	// Shader buffers
	writer.Write(state.mNumStorageBlocks);
	for (u8 i = 0; i < state.mNumStorageBlocks; ++i)
	{
		const RenderState::StorageBlock& buffer = state.mStorageBlocks[i];
		writer.Write(buffer.mNameHash);
		writer.Write(buffer.mHandle);
	}

	// Textures
	writer.Write(state.mNumTextures);
	for (u8 i = 0; i < state.mNumTextures; ++i)
	{
		const RenderState::Texture& texture = state.mTextures[i];
		writer.Write(texture.mNameHash);
		writer.Write(texture.mHandle);
	}

	// Images
	writer.Write(state.mNumImages);
	for (u8 i = 0; i < state.mNumImages; ++i)
	{
		const RenderState::Image& image = state.mImages[i];
		writer.Write(image.mNameHash);
		writer.Write(image.mHandle);

		u8 readBit = image.read ? 1 << 0 : 0;
		u8 writeBit = image.write ? 1 << 1 : 0;
		u8 readWrite = readBit | writeBit;

		writer.Write(readWrite);
	}

	// Render pass
	writer.Write(state.mRenderPass);

	// shader handle
	writer.Write(state.mShader);

	// Viewport
	writer.Write(state.mViewport.x);
	writer.Write(state.mViewport.y);
	writer.Write(state.mViewport.width);
	writer.Write(state.mViewport.height);

	// Depth Func
	writer.Write(state.mDepthFunc);

	// Blend Func
	writer.Write(state.mSrcBlendFunc);
	writer.Write(state.mDstBlendFunc);

	// CullFace
	writer.Write(state.mCullFace);

	// enable/disable booleans
	u8 depthWriteBit = 1 << 0;
	u8 colorWriteBit = 1 << 1;
	u8 alphaBlendBit = 1 << 2;
	u8 wireframeBit =  1 << 3;

	u8 toggles = (state.mDepthWriteEnabled ? depthWriteBit : 0) |
				 (state.mColorWriteEnabled ? colorWriteBit : 0) |
				 (state.mAlphaBlendEnabled ? alphaBlendBit : 0) |
				 (state.mWireFrame ? wireframeBit : 0);

	writer.Write(toggles);
}

FrameEncoder::FrameEncoder(ClientResources& resources, u64 virtualCommandListSize)
	: mMemory(static_cast<u8*>(malloc(virtualCommandListSize)))
	, mWriter(mMemory, virtualCommandListSize)
	, mResources(resources)
	, mNextPass(0)
	, mAllocator(nullptr)
{

}

FrameEncoder::~FrameEncoder()
{
	DEBUG_ASSERT(!mRecording, "Destroyed while recording frame!");
	free(mMemory);
}

void FrameEncoder::Begin(LinearAllocator* frameAllocator)
{
	DEBUG_ASSERT(!mRecording, "Must end begin recording before beginning");
	DEBUG_ASSERT(frameAllocator, "Allocator cannot be null!");
	mRecording = true;
	mAllocator = frameAllocator;
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
	u32 size = static_cast<u32>(strlen(pass.mName) + 1);
	mWriter.Write(Memory{ pass.mName, size });

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
	
	void* frameData = mAllocator->Allocate(size);
	memcpy(frameData, data, size);
	mWriter.Write(Memory{frameData, size});

	return clientHandle;
}

void FrameEncoder::UpdateIndexBuffer(graphics::IndexBufferHandle clientHandle, const void* data, u32 size, u32 offset)
{
	mWriter.Write(RenderCommand::UpdateIndexBuffer);
	mWriter.Write(clientHandle);

	void* frameData = mAllocator->Allocate(size);
	memcpy(frameData, data, size);
	
	mWriter.Write(Memory{ frameData, size });
	mWriter.Write(offset);
}

void FrameEncoder::DestroyIndexBuffer(graphics::IndexBufferHandle clientHandle)
{
	mWriter.Write(RenderCommand::DestroyIndexBuffer);
	mWriter.Write(clientHandle);
}

VertexBufferHandle FrameEncoder::CreateVertexBuffer(const void* data, u32 size)
{
	DEBUG_ASSERT(mRecording, "");

	VertexBufferHandle clientHandle = mResources.CreateVertexBuffer();

	mWriter.Write(RenderCommand::CreateVertexBuffer);
	mWriter.Write(clientHandle);

	void* frameData = mAllocator->Allocate(size);
	memcpy(frameData, data, size);

	mWriter.Write(Memory{frameData, size});
	return clientHandle;
}

void FrameEncoder::UpdateVertexBuffer(graphics::VertexBufferHandle clientHandle, const void* data, u32 size, u32 offset)
{
	mWriter.Write(RenderCommand::UpdateVertexBuffer);
	mWriter.Write(clientHandle);

	void* frameData = mAllocator->Allocate(size);
	memcpy(frameData, data, size);
	
	mWriter.Write(Memory{frameData, size});
	mWriter.Write(offset);
}

void FrameEncoder::DestroyVertexBuffer(graphics::VertexBufferHandle clientHandle)
{
	mWriter.Write(RenderCommand::DestroyVertexBuffer);
	mWriter.Write(clientHandle);
}

// Uniform Buffers ////////////////////////////////

UniformBufferHandle FrameEncoder::CreateUniformBuffer(const void* data, u32 size)
{
	DEBUG_ASSERT(mRecording, "");

	UniformBufferHandle clientHandle = mResources.CreateUniformBuffer();

	mWriter.Write(RenderCommand::CreateUniformBuffer);
	mWriter.Write(clientHandle);
	
	void* frameData = mAllocator->Allocate(size);
	if (data)
	{
		memcpy(frameData, data, size);
	}
	else 
	{
		memset(frameData, 0, size);
	}
	mWriter.Write(Memory{ frameData, size });

	return clientHandle;
}

void FrameEncoder::UpdateUniformBuffer(UniformBufferHandle clientHandle, const void* data, u32 size, u32 offset)
{
	DEBUG_ASSERT(mRecording, "");

	mWriter.Write(RenderCommand::UpdateUniformBuffer);
	mWriter.Write(clientHandle);
	
	void* frameData = mAllocator->Allocate(size);
	memcpy(frameData, data, size);
	mWriter.Write(Memory{ frameData, size });
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
	
	void* frameData = mAllocator->Allocate(size);
	if (data)
	{
		memcpy(frameData, data, size);
	}
	else
	{
		memset(frameData, 0, size);
	}
	mWriter.Write(Memory{ frameData, size });

	return clientHandle;
}

void FrameEncoder::UpdateShaderBuffer(graphics::ShaderBufferHandle clientHandle, const void* data, u32 size, u32 offset)
{
	mWriter.Write(RenderCommand::UpdateShaderBuffer);
	mWriter.Write(clientHandle);
	
	void* frameData = mAllocator->Allocate(size);
	memcpy(frameData, data, size);
	mWriter.Write(Memory{ frameData, size });
	mWriter.Write(offset);
}

void FrameEncoder::DestroyShaderBuffer(graphics::ShaderBufferHandle clientHandle)
{
	mWriter.Write(RenderCommand::DestroyShaderBuffer);
	mWriter.Write(clientHandle);
}

ShaderHandle FrameEncoder::CreateShader(const ShaderSourceDescription& desc)
{
	DEBUG_ASSERT(mRecording, "");
	mWriter.Write(RenderCommand::CreateShader);

	ShaderHandle clientHandle = mResources.CreateShader();

	mWriter.Write(clientHandle);

	DEBUG_ASSERT(desc.vertSrc, "No vertex shader!");
	DEBUG_ASSERT(desc.fragSrc, "No fragment shader!");
	if (desc.tessCtrlSrc || desc.tessEvalSrc) // must have both, or none
	{
		DEBUG_ASSERT(desc.tessCtrlSrc, "No tesselation control shader!");
		DEBUG_ASSERT(desc.tessEvalSrc, "No tesselation evaluation shader!");
	}

	auto allocSrc = [&](const char* src)
	{
		u32 len = static_cast<u32>((strlen(src) + 1));
		void* dst = mAllocator->Allocate(len);
		memcpy(dst, src, len);
		return (const char*)dst;
	};

	ShaderSourceDescription result{};
	result.vertSrc = allocSrc(desc.vertSrc);
	result.fragSrc = allocSrc(desc.fragSrc);
	if (desc.tessCtrlSrc && desc.tessEvalSrc) // must have both, or none
	{
		result.tessCtrlSrc = allocSrc(desc.tessCtrlSrc);
		result.tessEvalSrc = allocSrc(desc.tessEvalSrc);
	}
	if (desc.geoSrc)
	{
		result.geoSrc = allocSrc(desc.geoSrc);
	}
	
	mWriter.Write(result);

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

TextureHandle FrameEncoder::CreateTexture2D(const graphics::TextureDescription2D& desc)
{
	mWriter.Write(RenderCommand::CreateTexture2D);

	graphics::TextureHandle clientHandle = mResources.CreateTexture();
	mWriter.Write(clientHandle);

	WriteCreateTexture2D(desc, mWriter, *mAllocator);

	return clientHandle;
}

TextureHandle FrameEncoder::CreateTexture3D(const graphics::TextureDescription3D& desc)
{
	mWriter.Write(RenderCommand::CreateTexture3D);
	TextureHandle clientHandle = mResources.CreateTexture();

	mWriter.Write(clientHandle);
	mWriter.Write(desc.mWidth);
	mWriter.Write(desc.mHeight);
	mWriter.Write(desc.mDepth);

	mWriter.Write(desc.mDataSize);
	if (desc.mDataSize > 0)
	{
		DEBUG_ASSERT(desc.mDepth == desc.mData.size(), "must upload all data or no data");
		for (const auto& data : desc.mData)
		{
			void* frameData = mAllocator->Allocate(desc.mDataSize);
			memcpy(frameData, data, desc.mDataSize);
			
			mWriter.Write(Memory{frameData, desc.mDataSize});
		}
	}
	
	mWriter.Write(desc.mFormat);
	mWriter.Write(desc.mWrap);
	mWriter.Write(desc.mFilter);
	mWriter.Write(desc.mMipmaps);
	mWriter.Write(desc.mBorderColor);

	return clientHandle;
}

TextureHandle FrameEncoder::CreateCubemap(const graphics::CubemapDescription& desc)
{
	TextureHandle clientHandle = mResources.CreateTexture();
	mWriter.Write(RenderCommand::CreateCubemap);

	mWriter.Write(clientHandle);
	mWriter.Write(desc.mWidth);
	mWriter.Write(desc.mHeight);

	mWriter.Write(desc.mDataSize);
	if (desc.mDataSize > 0)
	{
		for (const auto& [face, data] : desc.mData)
		{
			mWriter.Write(face);
			void* frameData = mAllocator->Allocate(desc.mDataSize);
			memcpy(frameData, data, desc.mDataSize);
			mWriter.Write(Memory{ frameData, desc.mDataSize });
		}
	}

	mWriter.Write(desc.mFormat);
	mWriter.Write(desc.mWrap);
	mWriter.Write(desc.mFilter);

	return clientHandle;
}

void FrameEncoder::DestroyTexture(TextureHandle clientHandle)
{
	mWriter.Write(RenderCommand::DestroyTexture);
	mWriter.Write(clientHandle);
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
		WriteCreateTexture2D(desc.mTextures[i].mDescription, mWriter, *mAllocator);
		mWriter.Write(desc.mTextures[i].mAttachment);
	}

	DEBUG_ASSERT(result.mWidth  > 0, "Invalid framebuffer size!");
	DEBUG_ASSERT(result.mHeight > 0, "Invalid framebuffer size!");
	return result;
}

void FrameEncoder::DestroyFrameBuffer(graphics::FrameBufferHandle clientHandle)
{
	mWriter.Write(RenderCommand::DestroyFrameBuffer);
	mWriter.Write(clientHandle);
}

void FrameEncoder::DrawMesh(const MeshHandle handle, const RenderState& state)
{
	DEBUG_ASSERT(mRecording, "");

	mWriter.Write(RenderCommand::DrawMesh);
	mWriter.Write(handle);
	
	WriteRenderState(state, mWriter);
}

void FrameEncoder::DispatchCompute(const RenderState& state, u16 groupsX, u16 groupsY, u16 groupsZ)
{
	DEBUG_ASSERT(mRecording, "");

	mWriter.Write(RenderCommand::DispatchCompute);

	WriteRenderState(state, mWriter);
	mWriter.Write(groupsX);
	mWriter.Write(groupsY);
	mWriter.Write(groupsZ);
}

void FrameEncoder::IssueMemoryBarrier()
{
	DEBUG_ASSERT(mRecording, "");

	mWriter.Write(RenderCommand::IssueMemoryBarrier);
}
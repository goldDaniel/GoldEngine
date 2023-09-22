#include "FrameEncoder.h"

#include "RenderCommands.h"

using namespace graphics;
using namespace gold;

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
}

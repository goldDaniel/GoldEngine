#pragma once 

#include "core/Core.h"
#include "RenderTypes.h"

namespace gold
{
	class RenderResources
	{
	private:
		const static uint32_t kInvalidHandle = std::numeric_limits<uint32_t>::max();

		u32 mNextClientVertex;
		std::unordered_map<graphics::VertexBufferHandle, graphics::VertexBufferHandle> mVertexBuffers;

		u32 mNextClientIndex;
		std::unordered_map<graphics::IndexBufferHandle, graphics::IndexBufferHandle> mIndexBuffers;

		u32 mNextUniformBuffer;
		std::unordered_map<graphics::UniformBufferHandle, graphics::UniformBuffer> mUniformBuffers;

		u32 mNextShaderBuffer;
		std::unordered_map<graphics::ShaderBufferHandle, graphics::StorageBuffer> mShaderBuffers;

	public:

		graphics::VertexBufferHandle createVertexBuffer()
		{
			mVertexBuffers[{mNextClientVertex}] = { 0 };
			
			mNextClientVertex++;

			return { (mNextClientVertex - 1) };
		}

		graphics::IndexBufferHandle createIndexBuffer()
		{
			mIndexBuffers[{mNextClientIndex}] = { 0 };
			mNextClientIndex++;
			return { (mNextClientIndex - 1) };
		}

		graphics::UniformBufferHandle CreateUniformBuffer(u32 size)
		{
			mUniformBuffers[{mNextUniformBuffer}] = { 0, size };
			mNextUniformBuffer++;
			return { (mNextUniformBuffer - 1) };
		}

		graphics::ShaderBufferHandle CreateShaderBuffer(u32 size)
		{
			mShaderBuffers[{mNextShaderBuffer}] = { 0, size };
			mNextShaderBuffer++;
			return { (mNextShaderBuffer - 1) };
		}

		// returns "server side" mapping from "client side" 
		graphics::VertexBufferHandle get(graphics::VertexBufferHandle clientHandle)
		{
			DEBUG_ASSERT(mVertexBuffers.find(clientHandle) != mVertexBuffers.end(), "");
			return mVertexBuffers[clientHandle];
		}

		// returns "server side" mapping from "client side" 
		graphics::IndexBufferHandle get(graphics::IndexBufferHandle clientHandle)
		{
			DEBUG_ASSERT(mIndexBuffers.find(clientHandle) != mIndexBuffers.end(), "");
			return mIndexBuffers[clientHandle];
		}
	};
}
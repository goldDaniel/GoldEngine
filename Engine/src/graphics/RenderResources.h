#pragma once 

#include "core/Core.h"
#include "RenderTypes.h"

namespace gold
{
	class RenderResources 
	{
	private:
		
		u32 mNextClientVertex = 1;
		std::unordered_map<graphics::VertexBufferHandle, graphics::VertexBufferHandle> mVertexBuffers;

		u32 mNextClientIndex = 1;
		std::unordered_map<graphics::IndexBufferHandle, graphics::IndexBufferHandle> mIndexBuffers;

		u32 mNextShader = 1;
		std::unordered_map<graphics::ShaderHandle, graphics::ShaderHandle> mShaders;

		u32 mNextUniformBuffer = 1;
		std::unordered_map<graphics::UniformBufferHandle, graphics::UniformBuffer> mUniformBuffers;

		u32 mNextShaderBuffer = 1;
		std::unordered_map<graphics::ShaderBufferHandle, graphics::StorageBuffer> mShaderBuffers;

		
	public:

		graphics::VertexBufferHandle CreateVertexBuffer()
		{
			mVertexBuffers[{mNextClientVertex}] = { 0 };
			
			mNextClientVertex++;

			return { (mNextClientVertex - 1) };
		}

		graphics::IndexBufferHandle CreateIndexBuffer()
		{
			mIndexBuffers[{mNextClientIndex}] = { 0 };
			mNextClientIndex++;
			return { (mNextClientIndex - 1) };
		}

		graphics::ShaderHandle CreateShader()
		{
			mShaders[{mNextShader}] = { 0 };
			mNextShader++;

			return { (mNextShader - 1) };
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

		graphics::VertexBufferHandle& get(graphics::VertexBufferHandle clientHandle)
		{
			DEBUG_ASSERT(mVertexBuffers.find(clientHandle) != mVertexBuffers.end(), "");
			return mVertexBuffers[clientHandle];
		}
		
		///////////////////////////////////////////////////
		// returns "server side" mapping from "client side"
		/////////////////////////////////////////////////// 

		graphics::IndexBufferHandle& get(graphics::IndexBufferHandle clientHandle)
		{
			DEBUG_ASSERT(mIndexBuffers.find(clientHandle) != mIndexBuffers.end(), "");
			return mIndexBuffers[clientHandle];
		}

		graphics::UniformBuffer& get(graphics::UniformBufferHandle clientHandle)
		{
			DEBUG_ASSERT(mUniformBuffers.find(clientHandle) != mUniformBuffers.end(), "");
			return mUniformBuffers[clientHandle];
		}

		graphics::StorageBuffer& get(graphics::ShaderBufferHandle clientHandle)
		{
			DEBUG_ASSERT(mShaderBuffers.find(clientHandle) != mShaderBuffers.end(), "");
			return mShaderBuffers[clientHandle];
		}
	};
}
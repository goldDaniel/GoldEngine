#pragma once 

#include "core/Core.h"
#include "RenderTypes.h"

namespace gold
{
	class ClientResources
	{
	public: 
		virtual graphics::VertexBufferHandle CreateVertexBuffer() = 0;

		virtual graphics::IndexBufferHandle CreateIndexBuffer() = 0;

		virtual graphics::ShaderHandle CreateShader() = 0;

		virtual graphics::UniformBufferHandle CreateUniformBuffer(u32 size) = 0;

		virtual graphics::ShaderBufferHandle CreateShaderBuffer(u32 size) = 0;
	};


	///////////////////////////////////////////////////
	// returns "server side" mapping from "client side"
	/////////////////////////////////////////////////// 

	class ServerResources
	{
	public:
		virtual graphics::VertexBufferHandle& get(graphics::VertexBufferHandle clientHandle) = 0;


		virtual graphics::IndexBufferHandle& get(graphics::IndexBufferHandle clientHandle) = 0;

		virtual graphics::UniformBuffer& get(graphics::UniformBufferHandle clientHandle) = 0;

		virtual graphics::StorageBuffer& get(graphics::ShaderBufferHandle clientHandle) = 0;
	};

	class RenderResources : public ClientResources, public ServerResources
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

		graphics::VertexBufferHandle CreateVertexBuffer() override
		{
			mVertexBuffers[{mNextClientVertex}] = { 0 };
			
			mNextClientVertex++;

			return { (mNextClientVertex - 1) };
		}

		graphics::IndexBufferHandle CreateIndexBuffer() override
		{
			mIndexBuffers[{mNextClientIndex}] = { 0 };
			mNextClientIndex++;
			return { (mNextClientIndex - 1) };
		}

		graphics::ShaderHandle CreateShader() override
		{
			mShaders[{mNextShader}] = { 0 };
			mNextShader++;

			return { (mNextShader - 1) };
		}

		graphics::UniformBufferHandle CreateUniformBuffer(u32 size) override
		{
			mUniformBuffers[{mNextUniformBuffer}] = { 0, size };
			mNextUniformBuffer++;
			return { (mNextUniformBuffer - 1) };
		}

		graphics::ShaderBufferHandle CreateShaderBuffer(u32 size) override
		{
			mShaderBuffers[{mNextShaderBuffer}] = { 0, size };
			mNextShaderBuffer++;
			return { (mNextShaderBuffer - 1) };
		}

		graphics::VertexBufferHandle& get(graphics::VertexBufferHandle clientHandle) override
		{
			DEBUG_ASSERT(mVertexBuffers.find(clientHandle) != mVertexBuffers.end(), "");
			return mVertexBuffers[clientHandle];
		}

		graphics::IndexBufferHandle& get(graphics::IndexBufferHandle clientHandle) override
		{
			DEBUG_ASSERT(mIndexBuffers.find(clientHandle) != mIndexBuffers.end(), "");
			return mIndexBuffers[clientHandle];
		}

		graphics::UniformBuffer& get(graphics::UniformBufferHandle clientHandle) override
		{
			DEBUG_ASSERT(mUniformBuffers.find(clientHandle) != mUniformBuffers.end(), "");
			return mUniformBuffers[clientHandle];
		}

		graphics::StorageBuffer& get(graphics::ShaderBufferHandle clientHandle) override
		{
			DEBUG_ASSERT(mShaderBuffers.find(clientHandle) != mShaderBuffers.end(), "");
			return mShaderBuffers[clientHandle];
		}
	};
}
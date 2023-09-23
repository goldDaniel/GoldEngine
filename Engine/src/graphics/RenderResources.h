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

		virtual graphics::MeshHandle CreateMesh() = 0;

		virtual graphics::TextureHandle CreateTexture() = 0;
	};


	///////////////////////////////////////////////////
	// returns "server side" mapping from "client side"
	/////////////////////////////////////////////////// 

	class ServerResources
	{
	public:
		virtual graphics::VertexBufferHandle& get(graphics::VertexBufferHandle clientHandle) = 0;

		virtual graphics::IndexBufferHandle& get(graphics::IndexBufferHandle clientHandle) = 0;

		virtual graphics::UniformBufferHandle& get(graphics::UniformBufferHandle clientHandle) = 0;

		virtual graphics::ShaderBufferHandle& get(graphics::ShaderBufferHandle clientHandle) = 0;

		virtual graphics::ShaderHandle& get(graphics::ShaderHandle clientHandle) = 0;

		virtual graphics::MeshHandle& get(graphics::MeshHandle clientHandle) = 0;

		virtual graphics::TextureHandle& get(graphics::TextureHandle clientHandle) = 0;
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
		std::unordered_map<graphics::UniformBufferHandle, graphics::UniformBufferHandle> mUniformBuffers;

		u32 mNextShaderBuffer = 1;
		std::unordered_map<graphics::ShaderBufferHandle, graphics::ShaderBufferHandle> mShaderBuffers;

		u32 mNextMesh = 1;
		std::unordered_map<graphics::MeshHandle, graphics::MeshHandle> mMeshes;

		u32 mNextTexture = 1;
		std::unordered_map<graphics::TextureHandle, graphics::TextureHandle> mTextures;
		
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
			mUniformBuffers[{mNextUniformBuffer}] = { 0 };
			mNextUniformBuffer++;
			return { (mNextUniformBuffer - 1) };
		}

		graphics::ShaderBufferHandle CreateShaderBuffer(u32 size) override
		{
			mShaderBuffers[{mNextShaderBuffer}] = { 0 };
			mNextShaderBuffer++;
			return { (mNextShaderBuffer - 1) };
		}

		graphics::MeshHandle CreateMesh() override
		{
			mMeshes[{mNextMesh}] = { 0 };
			mNextMesh++;
			return { (mNextMesh - 1) };
		}

		graphics::TextureHandle CreateTexture() override
		{
			mTextures[{mNextTexture}] = { 0 };
			mNextTexture++;
			return { (mNextTexture - 1) };
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

		graphics::UniformBufferHandle& get(graphics::UniformBufferHandle clientHandle) override
		{
			DEBUG_ASSERT(mUniformBuffers.find(clientHandle) != mUniformBuffers.end(), "");
			return mUniformBuffers[clientHandle];
		}

		graphics::ShaderBufferHandle& get(graphics::ShaderBufferHandle clientHandle) override
		{
			DEBUG_ASSERT(mShaderBuffers.find(clientHandle) != mShaderBuffers.end(), "");
			return mShaderBuffers[clientHandle];
		}

		graphics::ShaderHandle& get(graphics::ShaderHandle clientHandle) override
		{
			DEBUG_ASSERT(mShaders.find(clientHandle) != mShaders.end(), "");
			return mShaders[clientHandle];
		}

		graphics::MeshHandle& get(graphics::MeshHandle clientHandle) override
		{
			DEBUG_ASSERT(mMeshes.find(clientHandle) != mMeshes.end(), "");
			return mMeshes[clientHandle];
		}

		graphics::TextureHandle& get(graphics::TextureHandle clientHandle) override
		{
			DEBUG_ASSERT(mTextures.find(clientHandle) != mTextures.end(), "");
			return mTextures[clientHandle];
		}
	};
}
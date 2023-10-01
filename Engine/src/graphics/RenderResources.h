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

		virtual graphics::UniformBufferHandle CreateUniformBuffer() = 0;

		virtual graphics::ShaderBufferHandle CreateShaderBuffer() = 0;

		virtual graphics::FrameBufferHandle CreateFrameBuffer() = 0;

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

		virtual graphics::FrameBufferHandle& get(graphics::FrameBufferHandle clientHandle) = 0;
	};

	template<typename T>
	class ResourceMapper
	{
	private:
		u32 mNextID = 1;
		std::unordered_map<T, T> mBufferMap;
	public:
		T Create()
		{
			mBufferMap[{mNextID}] = { 0 };
			return { mNextID++ };
		}

		T& Get(const T& clientHandle)
		{
			DEBUG_ASSERT(mBufferMap.find(clientHandle) != mBufferMap.end(), "");
			return mBufferMap[clientHandle];
		}
	};

	class RenderResources : public ClientResources, public ServerResources
	{
	private:
		
		ResourceMapper<graphics::VertexBufferHandle> mVertexBuffers;
		ResourceMapper<graphics::IndexBufferHandle> mIndexBuffers;
		ResourceMapper<graphics::ShaderHandle> mShaders;
		ResourceMapper<graphics::UniformBufferHandle> mUniformBuffers;
		ResourceMapper<graphics::ShaderBufferHandle> mShaderBuffers;
		ResourceMapper<graphics::MeshHandle> mMeshs;
		ResourceMapper<graphics::TextureHandle> mTextures;
		ResourceMapper<graphics::FrameBufferHandle> mFrameBuffers;

	public:

		// Client Side
		graphics::VertexBufferHandle CreateVertexBuffer() override { return mVertexBuffers.Create(); }

		graphics::IndexBufferHandle CreateIndexBuffer() override { return mIndexBuffers.Create(); }

		graphics::ShaderHandle CreateShader() override { return mShaders.Create(); }

		graphics::UniformBufferHandle CreateUniformBuffer() override { return mUniformBuffers.Create(); }

		graphics::ShaderBufferHandle CreateShaderBuffer() override { return mShaderBuffers.Create(); }

		graphics::MeshHandle CreateMesh() override { return mMeshs.Create(); }

		graphics::TextureHandle CreateTexture() override { return mTextures.Create(); }

		graphics::FrameBufferHandle CreateFrameBuffer() override { return mFrameBuffers.Create();  }


		// Server Side
		graphics::VertexBufferHandle& get(graphics::VertexBufferHandle clientHandle) override { return mVertexBuffers.Get(clientHandle); }

		graphics::IndexBufferHandle& get(graphics::IndexBufferHandle clientHandle) override { return mIndexBuffers.Get(clientHandle); }

		graphics::UniformBufferHandle& get(graphics::UniformBufferHandle clientHandle) override { return mUniformBuffers.Get(clientHandle); }

		graphics::ShaderBufferHandle& get(graphics::ShaderBufferHandle clientHandle) override { return mShaderBuffers.Get(clientHandle); }

		graphics::ShaderHandle& get(graphics::ShaderHandle clientHandle) override { return mShaders.Get(clientHandle); }

		graphics::MeshHandle& get(graphics::MeshHandle clientHandle) override { return mMeshs.Get(clientHandle); }

		graphics::TextureHandle& get(graphics::TextureHandle clientHandle) override { return mTextures.Get(clientHandle); }

		graphics::FrameBufferHandle& get(graphics::FrameBufferHandle clientHandle) override { return mFrameBuffers.Get(clientHandle); }
	};
}
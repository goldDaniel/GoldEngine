#pragma once

#include "core/Core.h"
#include "core/Util.h"

#include "RenderTypes.h"

namespace graphics
{
	class Renderer
	{
	public:
		~Renderer();

		void Init(void* windowHandle);
		void Destroy();

		void BeginFrame();
		void EndFrame();

		void SetBackBufferSize(int w, int h);

		u8 AddRenderPass(const RenderPass& description);
		u8 AddRenderPass(const char* name, ClearColor clearColor = ClearColor::NO, ClearDepth clearDepth = ClearDepth::NO);	
		u8 AddRenderPass(const char* name, FrameBuffer target, ClearColor clearColor = ClearColor::NO, ClearDepth clearDepth = ClearDepth::NO);

		// Uniform blocks ////////////////////////////////
		template<typename T>
		UniformBufferHandle CreateUniformBlock(const std::vector<T>& data)
		{
			return CreateUniformBlock(&data[0], data.size() * sizeof(T));
		}
		template<typename T>
		UniformBufferHandle CreateUniformBlock(const T& data)
		{
			return CreateUniformBlock(&data, sizeof(T));
		}
		UniformBufferHandle CreateUniformBlock(const void* data, uint32_t size);


		template<typename T>
		void UpdateUniformBlock(const std::vector<T>& data, UniformBufferHandle binding)
		{
			UpdateUniformBlock(&data[0], data.size() * sizeof(T), binding);
		}
		template<typename T>
		void UpdateUniformBlock(const T& data, UniformBufferHandle binding)
		{
			UpdateUniformBlock(&data, sizeof(T), binding);
		}
		void UpdateUniformBlock(const void* data, uint32_t size, UniformBufferHandle binding)
		{
			UpdateUniformBlock(data, size, 0, binding);
		}

		void UpdateUniformBlock(const void* data, uint32_t size, uint32_t offset, UniformBufferHandle binding);


		// Shader Storage Blocks ////////////////////
		template<typename T>
		ShaderBufferHandle CreateStorageBlock(const std::vector<T>& data)
		{
			return CreateStorageBlock(&data[0], data.size() * sizeof(T));
		}
		template<typename T>
		ShaderBufferHandle CreateStorageBlock(const T& data)
		{
			return CreateStorageBlock(&data, sizeof(T));
		}
		ShaderBufferHandle CreateStorageBlock(const void* data, uint32_t size);

		template<typename T>
		void UpdateStorageBlock(const std::vector<T>& data, ShaderBufferHandle binding)
		{
			UpdateStorageBlock(&data[0], data.size() * sizeof(T), binding);
		}
		template<typename T>
		void UpdateStorageBlock(const T& data, ShaderBufferHandle binding)
		{
			UpdateStorageBlock(&data, sizeof(T), binding);
		}
		void UpdateStorageBlock(const void* data, uint32_t size, ShaderBufferHandle binding);


		// vertex buffers //////////////////////////
		template<typename T>
		VertexBufferHandle CreateVertexBuffer(const std::vector<T>& data, BufferUsage usage = BufferUsage::STATIC)
		{
			return CreateVertexBuffer(&data[0], static_cast<uint32_t>(sizeof(T) * data.size()), usage);
		}
		VertexBufferHandle CreateVertexBuffer(const void* data = nullptr, uint32_t dataSize = 0, BufferUsage usage = BufferUsage::STATIC);

		template<typename T>
		void UpdateVertexBuffer(VertexBufferHandle handle, const std::vector<T>& data)
		{
			UpdateVertexBuffer(handle, &data[0], static_cast<uint32_t>(data.size() * sizeof(T)));
		}
		void UpdateVertexBuffer(VertexBufferHandle handle, const void* data, uint32_t dataSize);

		void DestroyVertexBuffer(VertexBufferHandle handle);


		// index buffers ////////////////////////////////////////
		template<typename T>
		IndexBufferHandle CreateIndexBuffer(const std::vector<T>& data, BufferUsage usage = BufferUsage::STATIC)
		{
			return CreateIndexBuffer(&data[0], static_cast<uint32_t>(sizeof(T) * data.size()), usage);
		}
		static IndexBufferHandle CreateIndexBuffer(const void* data = nullptr, uint32_t dataSize = 0, BufferUsage usage = BufferUsage::STATIC);

		template<typename T>
		void UpdateIndexBuffer(IndexBufferHandle handle, const std::vector<T>& data)
		{
			UpdateIndexBuffer(handle, &data[0], static_cast<uint32_t>(data.size() * sizeof(T)));
		}
		void UpdateIndexBuffer(IndexBufferHandle handle, const void* data, uint32_t dataSize);

		void DestroyIndexBuffer(IndexBufferHandle handle);

		// textures //////////////////////////////////////////////
		TextureHandle CreateTexture2D(const TextureDescription2D& description);
		TextureHandle CreateTexture3D(const TextureDescription3D& description);
		TextureHandle CreateCubemap(const CubemapDescription& description);

		void DestroyTexture(TextureHandle handle);

		template<typename T>
		void TextureReadback(const TextureHandle handle, std::vector<T>& readbackBuffer)
		{
			TextureReadback(handle, (u8*)(&readbackBuffer[0]), static_cast<uint32_t>(sizeof(T) * readbackBuffer.size()));
		}

		void TextureReadback(const TextureHandle handle, u8* buffer, uint32_t size);

		// frame buffers /////////////////////////////////////
		FrameBuffer CreateFramebuffer(const TextureDescription2D& description, FramebufferAttachment attachment);
		FrameBuffer CreateFramebuffer(const FrameBufferDescription& description);
		void DestroyFramebuffer(FrameBuffer buffer);

		// shaders /////////////////////////////////////////////
		ShaderHandle CreateShader(const char* vertexSrc, const char* fragSrc, const char* tessCtrlSrc = nullptr, const char* tessEvalSrc = nullptr);
		ShaderHandle CreateComputeShader(const char* src);
		void DestroyShader(ShaderHandle shader);

		// meshes //////////////////////////////////////////////
		MeshHandle CreateMesh(const MeshDescription& description);
		void DestroyMesh(MeshHandle mesh);

		void DrawMesh(MeshHandle mesh, const RenderState& state, std::function<void()> preAction = nullptr);
		void DrawMeshInstanced(MeshHandle mesh, const RenderState& state, VertexBufferHandle instanceData, uint32_t instanceCount, std::function<void()> preAction = nullptr);

		void DispatchCompute(const RenderState& state, uint16_t localX, uint16_t localY, uint16_t localZ, std::function<void()> preAction = nullptr);

		void ClearBackBuffer();
	};
}
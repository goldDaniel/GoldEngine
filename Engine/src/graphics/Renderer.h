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
		u8 AddRenderPass(const char* name, FrameBufferHandle target, ClearColor clearColor = ClearColor::NO, ClearDepth clearDepth = ClearDepth::NO);

		// Uniform Blocks ////////////////////////////////////////
		UniformBufferHandle CreateUniformBuffer(const void* data, uint32_t size);
		void UpdateUniformBuffer(const void* data, uint32_t size, uint32_t offset, UniformBufferHandle binding);
		void DestroyUniformBuffer(UniformBufferHandle handle);

		// Shader Storage Blocks //////////////////////////////////
		ShaderBufferHandle CreateShaderBuffer(const void* data, uint32_t size);
		void UpdateShaderBuffer(const void* data, uint32_t size, uint32_t offset, ShaderBufferHandle binding);
		void DestroyShaderBuffer(ShaderBufferHandle handle);

		// Vertex Buffers ////////////////////////////////////////
		VertexBufferHandle CreateVertexBuffer(const void* data = nullptr, uint32_t dataSize = 0, BufferUsage usage = BufferUsage::STATIC);
		void UpdateVertexBuffer(VertexBufferHandle handle, const void* data, uint32_t dataSize, u32 offset);
		void DestroyVertexBuffer(VertexBufferHandle hanOdle);

		// index buffers ////////////////////////////////////////
		IndexBufferHandle CreateIndexBuffer(const void* data = nullptr, uint32_t dataSize = 0, BufferUsage usage = BufferUsage::STATIC);
		void UpdateIndexBuffer(IndexBufferHandle handle, const void* data, uint32_t dataSize, u32 offset);
		void DestroyIndexBuffer(IndexBufferHandle handle);

		// textures //////////////////////////////////////////////
		TextureHandle CreateTexture2D(const TextureDescription2D& description);
		TextureHandle CreateTexture3D(const TextureDescription3D& description);
		TextureHandle CreateCubemap(const CubemapDescription& description);
		void DestroyTexture(TextureHandle handle);
		void GenerateMipMaps(TextureHandle handle);

		// frame buffers /////////////////////////////////////
		FrameBuffer CreateFramebuffer(const TextureDescription2D& description, FramebufferAttachment attachment);
		FrameBuffer CreateFramebuffer(const FrameBufferDescription& description);
		void DestroyFramebuffer(FrameBufferHandle buffer);

		// shaders /////////////////////////////////////////////
		ShaderHandle CreateShader(const ShaderSourceDescription& desc);
		ShaderHandle CreateComputeShader(const char* src);
		void DestroyShader(ShaderHandle shader);

		// meshes //////////////////////////////////////////////
		MeshHandle CreateMesh(const MeshDescription& description);
		void DestroyMesh(MeshHandle mesh);

		void DrawMesh(MeshHandle mesh, const RenderState& state, std::function<void()> preAction = nullptr);
		void DrawMeshInstanced(MeshHandle mesh, const RenderState& state, VertexBufferHandle instanceData, uint32_t instanceCount, std::function<void()> preAction = nullptr);

		void DispatchCompute(const RenderState& state, uint16_t groupsX, uint16_t groupsY, uint16_t groupsZ, std::function<void()> preAction = nullptr);
		void IssueMemoryBarrier();


		void ClearBackBuffer();

		PerfStats GetPerfStats() const;
	};
}
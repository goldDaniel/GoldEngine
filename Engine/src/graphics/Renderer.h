#pragma once

#include "core/Core.h"
#include "core/Util.h"

#include "RenderTypes.h"

namespace graphics
{
	class Renderer
	{
	public:
		enum Backend
		{
			OpenGL,
			Vulkan, 
		};

		static std::unique_ptr<Renderer> CreateRenderer(Backend backend);

		virtual ~Renderer() {}

		virtual void Init(void* windowHandle) = 0;
		virtual void Destroy() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void SetBackBufferSize(int w, int h) = 0;

		virtual u8 AddRenderPass(const RenderPass& description) = 0;
		virtual u8 AddRenderPass(const char* name, ClearColor clearColor = ClearColor::NO, ClearDepth clearDepth = ClearDepth::NO) = 0;
		virtual u8 AddRenderPass(const char* name, FrameBufferHandle target, ClearColor clearColor = ClearColor::NO, ClearDepth clearDepth = ClearDepth::NO) = 0;

		// Uniform Blocks ////////////////////////////////////////
		virtual UniformBufferHandle CreateUniformBuffer(const void* data, uint32_t size) = 0;
		virtual void UpdateUniformBuffer(const void* data, uint32_t size, uint32_t offset, UniformBufferHandle binding) = 0;
		virtual void DestroyUniformBuffer(UniformBufferHandle handle) = 0;

		// Shader Storage Blocks //////////////////////////////////
		virtual ShaderBufferHandle CreateShaderBuffer(const void* data, uint32_t size) = 0;
		virtual void UpdateShaderBuffer(const void* data, uint32_t size, uint32_t offset, ShaderBufferHandle binding) = 0;
		virtual void DestroyShaderBuffer(ShaderBufferHandle handle) = 0;

		// Vertex Buffers ////////////////////////////////////////
		virtual VertexBufferHandle CreateVertexBuffer(const void* data = nullptr, uint32_t dataSize = 0, BufferUsage usage = BufferUsage::STATIC) = 0;
		virtual void UpdateVertexBuffer(VertexBufferHandle handle, const void* data, uint32_t dataSize, u32 offset) = 0;
		virtual void DestroyVertexBuffer(VertexBufferHandle handle) = 0;

		// index buffers ////////////////////////////////////////
		virtual IndexBufferHandle CreateIndexBuffer(const void* data = nullptr, uint32_t dataSize = 0, BufferUsage usage = BufferUsage::STATIC) = 0;
		virtual void UpdateIndexBuffer(IndexBufferHandle handle, const void* data, uint32_t dataSize, u32 offset) = 0;
		virtual void DestroyIndexBuffer(IndexBufferHandle handle) = 0;

		// textures //////////////////////////////////////////////
		virtual TextureHandle CreateTexture2D(const TextureDescription2D& description) = 0;
		virtual TextureHandle CreateTexture3D(const TextureDescription3D& description) = 0;
		virtual TextureHandle CreateCubemap(const CubemapDescription& description) = 0;
		virtual void DestroyTexture(TextureHandle handle) = 0;
		virtual void GenerateMipMaps(TextureHandle handle) = 0;

		// frame buffers /////////////////////////////////////
		virtual FrameBuffer CreateFramebuffer(const TextureDescription2D& description, FramebufferAttachment attachment) = 0;
		virtual FrameBuffer CreateFramebuffer(const FrameBufferDescription& description) = 0;
		virtual void DestroyFramebuffer(FrameBufferHandle buffer) = 0;

		// shaders /////////////////////////////////////////////
		virtual ShaderHandle CreateShader(const ShaderSourceDescription& desc) = 0;
		virtual ShaderHandle CreateComputeShader(const char* src) = 0;
		virtual void DestroyShader(ShaderHandle shader) = 0;

		// meshes //////////////////////////////////////////////
		virtual MeshHandle CreateMesh(const MeshDescription& description) = 0;
		virtual void DestroyMesh(MeshHandle mesh) = 0;

		virtual void DrawMesh(MeshHandle mesh, const RenderState& state, std::function<void()> preAction = nullptr) = 0;
		virtual void DrawMeshInstanced(MeshHandle mesh, const RenderState& state, VertexBufferHandle instanceData, uint32_t instanceCount, std::function<void()> preAction = nullptr) = 0;

		virtual void DispatchCompute(const RenderState& state, uint16_t groupsX, uint16_t groupsY, uint16_t groupsZ, std::function<void()> preAction = nullptr) = 0;
		virtual void IssueMemoryBarrier() = 0;


		virtual void ClearBackBuffer() = 0;

		virtual PerfStats GetPerfStats() const = 0;
	};
}
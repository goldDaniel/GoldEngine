#pragma once 

#include "Renderer.h"

namespace graphics
{
class Renderer_GL : public Renderer
{
public:
	virtual ~Renderer_GL() override;

	virtual void Init(void* windowHandle) override;
	virtual void Destroy() override;

	virtual void BeginFrame() override;
	virtual void EndFrame() override;

	virtual void SetBackBufferSize(int w, int h) override;

	virtual u8 AddRenderPass(const RenderPass& description) override;
	virtual u8 AddRenderPass(const char* name, ClearColor clearColor = ClearColor::NO, ClearDepth clearDepth = ClearDepth::NO) override;
	virtual u8 AddRenderPass(const char* name, FrameBufferHandle target, ClearColor clearColor = ClearColor::NO, ClearDepth clearDepth = ClearDepth::NO) override;

	// Uniform Blocks ////////////////////////////////////////
	virtual UniformBufferHandle CreateUniformBuffer(const void* data, uint32_t size) override;
	virtual void UpdateUniformBuffer(const void* data, uint32_t size, uint32_t offset, UniformBufferHandle binding) override;
	virtual void DestroyUniformBuffer(UniformBufferHandle handle) override;

	// Shader Storage Blocks //////////////////////////////////
	virtual ShaderBufferHandle CreateShaderBuffer(const void* data, uint32_t size) override;
	virtual void UpdateShaderBuffer(const void* data, uint32_t size, uint32_t offset, ShaderBufferHandle binding) override;
	virtual void DestroyShaderBuffer(ShaderBufferHandle handle) override;

	// Vertex Buffers ////////////////////////////////////////
	virtual VertexBufferHandle CreateVertexBuffer(const void* data = nullptr, uint32_t dataSize = 0, BufferUsage usage = BufferUsage::STATIC) override;
	virtual void UpdateVertexBuffer(VertexBufferHandle handle, const void* data, uint32_t dataSize, u32 offset) override;
	virtual void DestroyVertexBuffer(VertexBufferHandle handle) override;

	// index buffers ////////////////////////////////////////
	virtual IndexBufferHandle CreateIndexBuffer(const void* data = nullptr, uint32_t dataSize = 0, BufferUsage usage = BufferUsage::STATIC) override;
	virtual void UpdateIndexBuffer(IndexBufferHandle handle, const void* data, uint32_t dataSize, u32 offset) override;
	virtual void DestroyIndexBuffer(IndexBufferHandle handle) override;

	// textures //////////////////////////////////////////////
	virtual TextureHandle CreateTexture2D(const TextureDescription2D& description) override;
	virtual TextureHandle CreateTexture3D(const TextureDescription3D& description) override;
	virtual TextureHandle CreateCubemap(const CubemapDescription& description) override;
	virtual void DestroyTexture(TextureHandle handle) override;
	virtual void GenerateMipMaps(TextureHandle handle) override;

	// frame buffers /////////////////////////////////////
	virtual FrameBuffer CreateFramebuffer(const TextureDescription2D& description, FramebufferAttachment attachment) override;
	virtual FrameBuffer CreateFramebuffer(const FrameBufferDescription& description) override;
	virtual void DestroyFramebuffer(FrameBufferHandle buffer) override;

	// shaders /////////////////////////////////////////////
	virtual ShaderHandle CreateShader(const ShaderSourceDescription& desc) override;
	virtual ShaderHandle CreateComputeShader(const char* src) override;
	virtual void DestroyShader(ShaderHandle shader) override;

	// meshes //////////////////////////////////////////////
	virtual MeshHandle CreateMesh(const MeshDescription& description) override;
	virtual void DestroyMesh(MeshHandle mesh) override;

	virtual void DrawMesh(MeshHandle mesh, const RenderState& state, std::function<void()> preAction = nullptr) override;
	virtual void DrawMeshInstanced(MeshHandle mesh, const RenderState& state, VertexBufferHandle instanceData, uint32_t instanceCount, std::function<void()> preAction = nullptr) override;

	virtual void DispatchCompute(const RenderState& state, uint16_t groupsX, uint16_t groupsY, uint16_t groupsZ, std::function<void()> preAction = nullptr) override;
	virtual void IssueMemoryBarrier() override;


	virtual void ClearBackBuffer() override;

	virtual PerfStats GetPerfStats() const override;
};
}

#include "Renderer.h"

#include "Renderer_GL.h"

namespace graphics
{

#pragma warning( push )
#pragma warning( disable : 4100)

	class Renderer_Null : public Renderer
	{
	public:
		virtual ~Renderer_Null() {};

		virtual void Init(void* windowHandle) override { UNUSED_VAR(windowHandle); };
		virtual void Destroy() override {};

		virtual void BeginFrame() override {};
		virtual void EndFrame() override {};

		virtual void SetBackBufferSize(int w, int h) override {};

		virtual u8 AddRenderPass(const RenderPass& description) override {return 0;};
		virtual u8 AddRenderPass(const char* name, ClearColor clearColor = ClearColor::NO, ClearDepth clearDepth = ClearDepth::NO) override { return 0; };
		virtual u8 AddRenderPass(const char* name, FrameBufferHandle target, ClearColor clearColor = ClearColor::NO, ClearDepth clearDepth = ClearDepth::NO) override { return 0; };

		// Uniform Blocks ////////////////////////////////////////
		virtual UniformBufferHandle CreateUniformBuffer(const void* data, uint32_t size) override { return { 0 }; };
		virtual void UpdateUniformBuffer(const void* data, uint32_t size, uint32_t offset, UniformBufferHandle binding) override {};
		virtual void DestroyUniformBuffer(UniformBufferHandle handle) override {};

		// Shader Storage Blocks //////////////////////////////////
		virtual ShaderBufferHandle CreateShaderBuffer(const void* data, uint32_t size) override { return { 0 }; };
		virtual void UpdateShaderBuffer(const void* data, uint32_t size, uint32_t offset, ShaderBufferHandle binding) override {};
		virtual void DestroyShaderBuffer(ShaderBufferHandle handle) override {};

		// Vertex Buffers ////////////////////////////////////////
		virtual VertexBufferHandle CreateVertexBuffer(const void* data = nullptr, uint32_t dataSize = 0, BufferUsage usage = BufferUsage::STATIC) override { return { 0 }; };
		virtual void UpdateVertexBuffer(VertexBufferHandle handle, const void* data, uint32_t dataSize, u32 offset) override {};
		virtual void DestroyVertexBuffer(VertexBufferHandle handle) override {};

		// index buffers ////////////////////////////////////////
		virtual IndexBufferHandle CreateIndexBuffer(const void* data = nullptr, uint32_t dataSize = 0, BufferUsage usage = BufferUsage::STATIC) override { return { 0 }; };
		virtual void UpdateIndexBuffer(IndexBufferHandle handle, const void* data, uint32_t dataSize, u32 offset) override {};
		virtual void DestroyIndexBuffer(IndexBufferHandle handle) override {};

		// textures //////////////////////////////////////////////
		virtual TextureHandle CreateTexture2D(const TextureDescription2D& description) override { return { 0 }; };
		virtual TextureHandle CreateTexture3D(const TextureDescription3D& description) override { return { 0 }; };
		virtual TextureHandle CreateCubemap(const CubemapDescription& description) override { return { 0 }; };
		virtual void DestroyTexture(TextureHandle handle) override {};
		virtual void GenerateMipMaps(TextureHandle handle) override {};

		// frame buffers /////////////////////////////////////
		virtual FrameBuffer CreateFramebuffer(const TextureDescription2D& description, FramebufferAttachment attachment) override { return { 0 }; };
		virtual FrameBuffer CreateFramebuffer(const FrameBufferDescription& description) override { return { 0 }; };
		virtual void DestroyFramebuffer(FrameBufferHandle buffer) override {};

		// shaders /////////////////////////////////////////////
		virtual ShaderHandle CreateShader(const ShaderSourceDescription& desc) override { return { 0 }; };
		virtual ShaderHandle CreateComputeShader(const char* src) override { return { 0 }; };
		virtual void DestroyShader(ShaderHandle shader) override {};

		// meshes //////////////////////////////////////////////
		virtual MeshHandle CreateMesh(const MeshDescription& description) override { return { 0 }; };
		virtual void DestroyMesh(MeshHandle mesh) override {};

		virtual void DrawMesh(MeshHandle mesh, const RenderState& state, std::function<void()> preAction = nullptr) override {};
		virtual void DrawMeshInstanced(MeshHandle mesh, const RenderState& state, VertexBufferHandle instanceData, uint32_t instanceCount, std::function<void()> preAction = nullptr) override {};

		virtual void DispatchCompute(const RenderState& state, uint16_t groupsX, uint16_t groupsY, uint16_t groupsZ, std::function<void()> preAction = nullptr) override {};
		virtual void IssueMemoryBarrier() override {};


		virtual void ClearBackBuffer() override {};

		virtual PerfStats GetPerfStats() const override { return PerfStats{}; };
	};
#pragma warning( pop )

	std::unique_ptr<Renderer> Renderer::CreateRenderer(Backend backend)
	{
		switch (backend)
		{
		case OpenGL:
			return std::make_unique<Renderer_GL>();
		default:
			return std::make_unique<Renderer_Null>();
		}
	}
}
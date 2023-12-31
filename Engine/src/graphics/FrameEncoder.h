#pragma once 

#include "core/Core.h"

#include "memory/BinaryWriter.h"
#include "memory/LinearAllocator.h"

#include "RenderTypes.h"
#include "RenderResources.h"

namespace gold
{
	class FrameEncoder
	{
	private:
		bool mRecording = false;
		u8* mMemory;

		LinearAllocator* mAllocator;
		ClientResources& mResources;
		BinaryWriter mWriter;

		// TODO (danielg): Move this somewhere else? it needs to be reset every frame
		u8 mNextPass{};

	public:
		FrameEncoder(ClientResources& resources, u64 virtualCommandListSize);

		~FrameEncoder();

		bool ReadyForRead() const { return !mRecording && mWriter.GetOffset() > 0; };

		void Begin(LinearAllocator* allocator);

		void End();

		BinaryReader GetReader();

		u8 AddRenderPass(const graphics::RenderPass& pass);
		u8 AddRenderPass(const char* name, graphics::FrameBufferHandle target, graphics::ClearColor color, graphics::ClearDepth depth);
		u8 AddRenderPass(const char* name, graphics::ClearColor color, graphics::ClearDepth depth);

		graphics::IndexBufferHandle CreateIndexBuffer(const void* data, u32 size);
		void UpdateIndexBuffer(graphics::IndexBufferHandle clientHandle, const void* data, u32 size, u32 offset = 0);
		void DestroyIndexBuffer(graphics::IndexBufferHandle clientHandle);

		graphics::VertexBufferHandle CreateVertexBuffer(const void* data, u32 size);
		void UpdateVertexBuffer(graphics::VertexBufferHandle clientHandle, const void* data, u32 size, u32 offset = 0);
		void DestroyVertexBuffer(graphics::VertexBufferHandle clientHandle);

		graphics::UniformBufferHandle CreateUniformBuffer(const void* data, u32 size);
		void UpdateUniformBuffer(graphics::UniformBufferHandle clientHandle, const void* data, u32 size, u32 offset = 0);
		void DestroyUniformBuffer(graphics::UniformBufferHandle clientHandle);

		graphics::ShaderBufferHandle CreateShaderBuffer(const void* data, u32 size);
		void UpdateShaderBuffer(graphics::ShaderBufferHandle clientHandle, const void* data, u32 size, u32 offset = 0);
		void DestroyShaderBuffer(graphics::ShaderBufferHandle clientHandle);

		graphics::ShaderHandle CreateShader(const graphics::ShaderSourceDescription& desc);

		graphics::MeshHandle CreateMesh(const graphics::MeshDescription& mesh);

		graphics::TextureHandle CreateTexture2D(const graphics::TextureDescription2D& desc);
		graphics::TextureHandle CreateTexture3D(const graphics::TextureDescription3D& desc);
		graphics::TextureHandle CreateCubemap(const graphics::CubemapDescription& desc);
		void DestroyTexture(graphics::TextureHandle clientHandle);
		void GenerateMipMaps(graphics::TextureHandle clientHandle);

		graphics::FrameBuffer CreateFrameBuffer(const graphics::FrameBufferDescription& desc);
		void DestroyFrameBuffer(graphics::FrameBufferHandle handle);

		void DrawMesh(const graphics::MeshHandle mesh, const graphics::RenderState& state);

		void DispatchCompute(const graphics::RenderState& state, u16 groupsX, u16 groupsY, u16 groupsZ);

		void IssueMemoryBarrier();
	};
}
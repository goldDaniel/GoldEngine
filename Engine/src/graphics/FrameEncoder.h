#pragma once 

#include "core/Core.h"

#include "memory/BinaryWriter.h"

#include "RenderTypes.h"
#include "RenderResources.h"

namespace gold
{
	class FrameEncoder
	{
	private:
		bool mRecording = false;
		u8* mMemory;

		ClientResources& mResources;
		BinaryWriter mWriter;

		// TODO (danielg): Move this somewhere else? it needs to be reset every frame
		u8 mNextPass{};

	public:
		FrameEncoder(ClientResources& resources, u64 virtualCommandListSize);

		~FrameEncoder();

		bool ReadyForRead() const { return !mRecording && mWriter.GetOffset() > 0; };

		void Begin();

		void End();

		BinaryReader GetReader();

		u8 AddRenderPass(const graphics::RenderPass& pass);
		u8 AddRenderPass(const char* name, graphics::FrameBuffer target, graphics::ClearColor color, graphics::ClearDepth depth);
		u8 AddRenderPass(const char* name, graphics::ClearColor color, graphics::ClearDepth depth);

		graphics::IndexBufferHandle CreateIndexBuffer(const void* data, u32 size);

		graphics::VertexBufferHandle CreateVertexBuffer(const void* data, u32 size);

		graphics::UniformBufferHandle CreateUniformBuffer(const void* data, u32 size);
		void UpdateUniformBuffer(graphics::UniformBufferHandle clientHandle, const void* data, u32 size, u32 offset = 0);
		void DestroyUniformBuffer(graphics::UniformBufferHandle clientHandle);

		graphics::ShaderBufferHandle CreateShaderBuffer(const void* data, u32 size);
		void UpdateShaderBuffer(graphics::ShaderBufferHandle clientHandle, const void* data, u32 size, u32 offset = 0);

		graphics::ShaderHandle CreateShader(const char* vertSrc, const char* fragSrc);

		graphics::MeshHandle CreateMesh(const graphics::MeshDescription& mesh);

		graphics::TextureHandle CreateTexture2D(const graphics::TextureDescription2D& desc);

		void DrawMesh(const graphics::MeshHandle mesh, const graphics::RenderState& state);
	};
}
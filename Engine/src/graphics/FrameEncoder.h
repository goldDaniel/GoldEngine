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
		static constexpr u64 kSize = 1024 * 1024 * 128;
		u8* mMemory;

		ClientResources& mResources;
		BinaryWriter mWriter;

		// TODO (danielg): Move this somewhere else? it needs to be reset every frame
		u8 mNextPass{};

		bool mReadReady{};

	public:
		FrameEncoder(ClientResources& resources);

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

		graphics::ShaderBufferHandle CreateShaderBuffer(const void* data, u32 size);

		graphics::ShaderHandle CreateShader(const char* vertSrc, const char* fragSrc);

		graphics::MeshHandle CreateMesh(const graphics::MeshDescription& mesh);

		void DrawMesh(const graphics::MeshHandle mesh, const graphics::RenderState& state);
	};
}
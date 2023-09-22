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
		static constexpr u64 size = 1024 * 1024 * 128;
		u8* mMemory;

		RenderResources& mResources;
		BinaryWriter mWriter;

	public:
		FrameEncoder(RenderResources& resources);

		~FrameEncoder();

		void Begin();

		void End();

		graphics::IndexBufferHandle CreateIndexBuffer(const void* data, u64 size);

		graphics::VertexBufferHandle CreateVertexBuffer(const void* data, u64 size);

		graphics::UniformBufferHandle CreateUniformBuffer(const void* data, u64 size);

		graphics::ShaderBufferHandle CreateShaderBuffer(const void* data, u64 size);

		void DrawMesh(const graphics::MeshHandle mesh, const graphics::RenderState& state);
	};
}
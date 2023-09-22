#pragma once

#include "core/Core.h"

namespace gold
{
	enum struct RenderCommand : u8
	{
		CreateUniformBuffer = 0,
		UpdateUniformBuffer,
		DestroyUniformBuffer,

		CreateShaderBuffer,
		UpdateShaderBuffer,
		DestroyShaderBuffer,

		CreateVertexBuffer,
		UpdateVertexBuffer,
		DestroyVertexBuffer,

		CreateIndexBuffer,
		UpdateIndexBuffer,
		DestroyIndexBuffer,

		CreateTexture2D,
		CreateTexture3D,
		CreateCubemap,
		DestroyTexture,

		CreateFrameBuffer,
		DestroyFrameBuffer,

		DrawMesh,
		DrawMeshInstanced,
		DispatchCompute,

		END,
	};
}
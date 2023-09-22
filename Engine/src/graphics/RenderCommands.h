#pragma once

#include "core/Core.h"

namespace gold
{
	enum struct RenderCommand : u8
	{
		CreateUniformBuffer = 0, //e
		UpdateUniformBuffer,
		DestroyUniformBuffer,

		CreateShaderBuffer, //e
		UpdateShaderBuffer,
		DestroyShaderBuffer,

		CreateVertexBuffer,	//e
		UpdateVertexBuffer,
		DestroyVertexBuffer,

		CreateIndexBuffer,	//e
		UpdateIndexBuffer,
		DestroyIndexBuffer,

		CreateShader, //e
		DestroyShader,

		CreateTexture2D,
		CreateTexture3D,
		CreateCubemap,
		DestroyTexture,

		CreateFrameBuffer,
		DestroyFrameBuffer,

		DrawMesh,			//e
		DrawMeshInstanced,
		DispatchCompute,

		AddRenderPass, //e

		END,
	};
}
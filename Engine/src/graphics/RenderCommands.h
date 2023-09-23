#pragma once

#include "core/Core.h"

namespace gold
{
	enum struct RenderCommand : u8
	{
		CreateUniformBuffer = 0, //e, d
		UpdateUniformBuffer,
		DestroyUniformBuffer,

		CreateShaderBuffer, //e, d
		UpdateShaderBuffer,
		DestroyShaderBuffer,

		CreateVertexBuffer,	//e, d
		UpdateVertexBuffer,
		DestroyVertexBuffer,

		CreateIndexBuffer,	//e, d
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

		END, //e, d
	};
}
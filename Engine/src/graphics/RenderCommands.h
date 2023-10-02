#pragma once

#include "core/Core.h"

namespace gold
{
	enum struct RenderCommand : u8
	{
		CreateUniformBuffer = 0, //e, d
		UpdateUniformBuffer, //e, d
		DestroyUniformBuffer, //e, d

		CreateShaderBuffer, //e, d
		UpdateShaderBuffer, //e, d
		DestroyShaderBuffer, //e, d

		CreateVertexBuffer,	//e, d
		UpdateVertexBuffer, //e, d
		DestroyVertexBuffer, //e, d

		CreateIndexBuffer,	//e, d
		UpdateIndexBuffer, //e, d
		DestroyIndexBuffer, //e, d 

		CreateShader, //e, d
		DestroyShader,

		CreateTexture2D, //e, d
		CreateTexture3D, //e, d
		CreateCubemap, //e, d
		DestroyTexture, //e, d

		CreateFrameBuffer, //e, d
		DestroyFrameBuffer, //e, D

		CreateMesh, //e, d

		DrawMesh,			//e, d
		DrawMeshInstanced,
		DispatchCompute,

		AddRenderPass, //e, d

		END, //e, d
	};
}
#pragma once

#include "core/Core.h"

namespace gold
{
	enum struct RenderCommand : u8
	{
		CreateUniformBuffer = 0, //e, d
		/*UpdateUniformBuffer,
		DestroyUniformBuffer,*/

		CreateShaderBuffer, //e, d
		/*UpdateShaderBuffer,
		DestroyShaderBuffer,*/

		CreateVertexBuffer,	//e, d
		/*UpdateVertexBuffer,
		DestroyVertexBuffer,*/

		CreateIndexBuffer,	//e, d
		/*UpdateIndexBuffer,
		DestroyIndexBuffer,*/

		CreateShader, //e, d
		/*DestroyShader,*/

		/*CreateTexture2D,
		CreateTexture3D,
		CreateCubemap,
		DestroyTexture,*/

		/*CreateFrameBuffer,
		DestroyFrameBuffer,*/

		CreateMesh, //e, d

		DrawMesh,			//e, d
		/*DrawMeshInstanced,
		DispatchCompute,*/

		AddRenderPass, //e, d

		END, //e, d
	};
}
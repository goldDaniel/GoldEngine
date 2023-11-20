#pragma once

#include "graphics/RenderTypes.h"

namespace graphics
{
	struct MaterialHandle
	{
		u32 idx{};
	};

	struct Material
	{
		glm::vec4 albedo{ 0.4f, 1.0f, 1.0f, 1.0f };
		glm::vec4 emissive{ 0.f, 0.f, 0.f, 0.0f };

		//metallic, roughness, ?, UVScale
		glm::vec4 coefficients{0.2f, 0.8f, 0.0f, 1.0f};

		// albedo, normal, metallic, roughness
		glm::vec4 mapFlags{};
	};
}
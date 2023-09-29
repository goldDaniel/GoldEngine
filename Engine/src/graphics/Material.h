#pragma once

#include "graphics/RenderTypes.h"

namespace graphics
{
	struct MaterialHandle
	{
		u32 idx;
	};

	struct Material
	{
		glm::vec4 albedo{ 0.4f, 1.0f, 1.0f, 1.0f };
		glm::vec4 emissive{ 0.f, 0.f, 0.f, 0.0f };

		float metallic = 0.2f;
		float roughness = 0.8f;

		bool useAlbedoMap = false;
		bool useNormalMap = false;
		bool useMetallicMap = false;
		bool useRoughnessMap = false;

		float uvScale = 1.0f;

		graphics::TextureHandle albedoMap{};
		graphics::TextureHandle normalMap{};
		graphics::TextureHandle metallicMap{};
		graphics::TextureHandle roughnessMap{};
	};
}
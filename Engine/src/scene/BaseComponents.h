#pragma once 

#include "core/Core.h"
#include "SceneGraph.h"

#include "graphics/Renderer.h"
#include "graphics/Material.h"

struct ChildrenComponent
{
	std::vector<entt::entity> children;
};

struct ParentComponent
{
	entt::entity parent;
};

struct TagComponent
{
	std::string tag;
};

struct TransformComponent
{
	glm::vec3 prevPosition{ 0 };
	glm::vec3 position{ 0 };

	glm::vec3 rotation{};
	glm::vec3 scale{ 1,1,1 };

	glm::mat4 GetMatrix() const
	{
		return glm::translate(position) * glm::toMat4(glm::quat(rotation)) * glm::scale(scale);
	}

	glm::mat4 GetInterpolatedMatrix(float alpha) const
	{
		glm::vec3 pos = position;
		if (glm::dot(prevPosition, prevPosition) > 0)
		{
			pos = position * alpha + prevPosition * (1.0f - alpha);
		}

		return glm::translate(pos) * glm::toMat4(glm::quat(rotation)) * glm::scale(scale);
	}
};

struct LightComponent
{
	enum Type { Directional, Point };

	Type type = Directional;
	glm::vec3 color;
};

struct NotFrustumCulledComponent
{
	u8 flag;
};

struct DirectionalLightComponent
{
	glm::vec4 direction{ 0 };
	glm::vec4 color{ 0 };
};

struct PointLightComponent
{
	glm::vec4 color{ 0 };
	float falloff = 0;
};

struct ShadowMapComponent
{
	// NOTE (danielg): view matrix will be calculated during pass 

	union
	{
		// construct dir light ortho
		struct
		{
			float left;
			float right;
			float top;
			float bottom;
		};

		// construct point light projection
		struct
		{
			float FOV;
			float aspect;
		};
	};

	float nearPlane;
	float farPlane;

	// NOTE (danielg): 0 is used for directional lights. Otherwise: 
	/*
		0 - ShadowMapIndex+X
		1 - shadowMapIndex-X
		2 - shadowMapIndex+Y,
		3 - shadowMapIndex-Y,
		4 - shadowMapIndex+Z,
		5 - shadowMapIndex-Z
	*/
	std::array<float, 6> shadowMapBias{ -1,-1,-1,-1,-1,-1 };
	std::array<int, 6> shadowMapIndex{ -1,-1,-1,-1,-1,-1 };
};

struct RenderComponent
{
	graphics::MeshHandle mesh{};

	// TODO (danielg): have material buffer so renderComponent only needs an index/handle
	//graphics::MaterialHandle material{};

	glm::vec3 aabbMin{};
	glm::vec3 aabbMax{};


	glm::vec4 albedo{ 0.4f, 1.0f, 1.0f, 1.0f };
	glm::vec4 emissive{ 0.f, 0.f, 0.f, 0.0f };

	float metallic = 0.0f;
	float roughness = 1.0f;
	float uvScale = 1.0f;

	graphics::TextureHandle albedoMap{ 0 };
	graphics::TextureHandle normalMap{ 0 };
	graphics::TextureHandle metallicMap{ 0 };
	graphics::TextureHandle roughnessMap{ 0 };
};

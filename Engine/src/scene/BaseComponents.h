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
	glm::vec4 direction{ 0,0,0,1 }; // xyz, dirty flag
	glm::vec4 color{ 1,1,1,1 };
};

struct PointLightComponent
{
	glm::vec4 color{ 0 }; //rgb, dirty flag
	float falloff = 0;
};

struct ShadowMapComponent
{
	// NOTE (danielg): view matrix will be calculated during pass 

	union
	{
		// construct dir light ortho
		struct Ortho
		{
			float left;
			float right;
			float top;
			float bottom;
		} ortho;

		// construct point light projection
		struct Perspective
		{
			float FOV;
			float aspect;
		} perspective;
	};
	
	float nearPlane;
	float farPlane;

	int PCFSize{ 1 };

	bool dirty{ true };

	// NOTE (danielg): 0 is used for directional lights. Otherwise: 
	/*
		0 - ShadowMapIndex+X
		1 - shadowMapIndex-X
		2 - shadowMapIndex+Y,
		3 - shadowMapIndex-Y,
		4 - shadowMapIndex+Z,
		5 - shadowMapIndex-Z
	*/

	std::array<float, 6> shadowMapBias{ 0,0,0,0,0,0 };
	std::array<int, 6> shadowMapIndex{ -1,-1,-1,-1,-1,-1 };
};

struct LightBufferComponent
{
	// 1 per directional light
	// 6 per point light 
	static constexpr u32 MAX_CASTERS = 64;

	struct DirectionalLight
	{
		glm::vec4 direction{};
		glm::vec4 color{};

		glm::ivec4 params{}; // ?, ?, ?, shadowMapIndex
	};

	struct PointLight
	{
		glm::vec4 position;
		glm::vec4 color;
		glm::ivec4 params0; // falloff, ?, shadowMapIndex+X, shadowMapIndex-X
		glm::ivec4 params1; // shadowMapIndex+Y, shadowMapIndex-Y, shadowMapIndex+Z, shadowMapIndex-Z
	};

	struct LightShaderBuffer
	{
		glm::ivec4 lightCounts{}; //directional, point, ?, ?

		DirectionalLight directionalLights[MAX_CASTERS]{};
		PointLight pointLights[MAX_CASTERS]{};
	};

	LightShaderBuffer lightBuffer{};
	bool isDirty = true;
};

struct RenderComponent
{
	glm::vec3 aabbMin{};
	glm::vec3 aabbMax{};

	graphics::MeshHandle mesh{};
	graphics::MaterialHandle material{};
};

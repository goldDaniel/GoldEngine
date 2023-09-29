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

	glm::vec3 rotation;
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

struct RenderComponent
{
	graphics::MeshHandle mesh{};
	graphics::MaterialHandle material{};
};

#include "GameObject.h"
#include "BaseComponents.h"

#include <glm/gtx/transform.hpp>

#include "BaseComponents.h"

using namespace scene;

void GameObject::ForEachChild(std::function<void(GameObject&)> func)
{
	if (HasComponent<ChildrenComponent>())
	{
		auto comp = GetComponent<ChildrenComponent>();
		for (auto child : comp.children)
		{
			func(GameObject(mRegistry, child));
		}
	}
}

void GameObject::ForEachChild(std::function<void(const GameObject&)> func) const
{
	if (HasComponent<ChildrenComponent>())
	{
		auto comp = GetComponent<ChildrenComponent>();
		for (auto child : comp.children)
		{
			func(GameObject(mRegistry, child));
		}
	}
}

void GameObject::RemoveChildren(std::vector<GameObject> toRemove)
{
	if (HasComponent<ChildrenComponent>())
	{
		auto& children = GetComponent<ChildrenComponent>().children;

		for (auto& obj : toRemove)
		{
			children.erase(std::remove(children.begin(), children.end(), obj.GetEntity()), children.end());
		}

		if (children.empty())
		{
			RemoveComponent<ChildrenComponent>();
		}
	}

}

void GameObject::SetParent(GameObject parent)
{
	if (!parent.IsValid())
	{
		if (HasComponent<ParentComponent>())
		{
			RemoveComponent<ParentComponent>();
		}

		return;
	}

	if (HasComponent<ParentComponent>())
	{
		auto thisParent = GetComponent<ParentComponent>().parent;
		GameObject(mRegistry, thisParent).RemoveChildren({ *this });
	}
	else
	{
		AddComponent<ParentComponent>();
	}

	auto& thisParent = GetComponent<ParentComponent>().parent;
	thisParent = parent.mEntity;

	if (HasParent())
	{
		GameObject(mRegistry, thisParent)._AddChild(*this);
	}
}

bool GameObject::IsValid() const
{
	return mRegistry != nullptr && mEntity != entt::null;
}

void GameObject::Destroy()
{
	if (!mRegistry) return;

	if (HasParent())
	{
		GameObject(mRegistry, GetComponent<ParentComponent>().parent).RemoveChildren({ *this });
	}

	if (HasComponent<ChildrenComponent>())
	{
		auto& children = GetComponent<ChildrenComponent>().children;

		for (auto& child : children)
		{
			entt::entity parent = entt::null;
			if (HasParent())
			{
				parent = GetComponent<ParentComponent>().parent;
			}

			GameObject(mRegistry, child).SetParent(GameObject(mRegistry, parent));
		}
	}

	mRegistry->destroy(mEntity);
}

bool GameObject::HasParent() const { return HasComponent<ParentComponent>() && GameObject(mRegistry, GetComponent<ParentComponent>().parent).IsValid(); }

void GameObject::SetTag(const std::string& tag)
{
	auto& component = mRegistry->get<TagComponent>(mEntity);
	component.tag = tag;
}

void GameObject::_AddChild(GameObject child)
{
	if (HasComponent<ChildrenComponent>())
	{
		auto& children = GetComponent<ChildrenComponent>().children;
		for (const auto& obj : children)
		{
			DEBUG_ASSERT(obj != child.GetEntity(), "");
		}

		children.push_back(child.mEntity);
	}
	else
	{
		AddComponent<ChildrenComponent>().children.push_back(child.GetEntity());
	}


}

glm::mat4 GameObject::GetWorldSpaceTransform() const
{
	const auto& component = mRegistry->get<TransformComponent>(mEntity);

	glm::mat4 result = glm::translate(component.position);
	result *= glm::toMat4(glm::quat(component.rotation));
	result *= glm::scale(component.scale);

	if (HasParent())
	{
		return GameObject(mRegistry, GetComponent<ParentComponent>().parent).GetWorldSpaceTransform() * result;
	}

	return result;
}

glm::mat4 GameObject::GetInterpolatedWorldSpaceTransform(float alpha) const
{
	const auto& component = mRegistry->get<TransformComponent>(mEntity);
	glm::mat4 interpolated = component.GetInterpolatedMatrix(alpha);

	if (HasParent())
	{
		return GameObject(mRegistry, GetComponent<ParentComponent>().parent).GetInterpolatedWorldSpaceTransform(alpha) * interpolated;
	}

	return interpolated;
}

std::tuple<glm::vec4, glm::vec4> GameObject::GetAABB() const
{
	glm::vec3 min(std::numeric_limits<float>::max());
	glm::vec3 max(std::numeric_limits<float>::min());

	ForEachChild([&min, &max](const GameObject& obj)
		{
			//TODO (danielg): AABBS have been ripped out temporarily during refactor
			//if (obj.HasComponent<MeshComponent>())
			{
				const auto [aabbMin, aabbMax] = obj.GetAABB();

				if (aabbMin.x < min.x) min.x = aabbMin.x;
				if (aabbMin.y < min.y) min.y = aabbMin.y;
				if (aabbMin.z < min.z) min.z = aabbMin.z;

				if (aabbMax.x > max.x) max.x = aabbMax.x;
				if (aabbMax.y > max.y) max.y = aabbMax.y;
				if (aabbMax.z > max.z) max.z = aabbMax.z;
			}
		});


	const auto& transform = GetComponent<TransformComponent>();

	min *= transform.scale;
	min += transform.position;

	max *= transform.scale;
	max += transform.position;

	return { glm::vec4(min, 1), glm::vec4(max,1) };
}

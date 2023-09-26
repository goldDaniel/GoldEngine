#pragma once

#include <entt/entt.hpp>

#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <string>
#include <memory>


namespace scene
{
	class GameObject
	{
	private:
		entt::registry* mRegistry = nullptr;
		entt::entity mEntity = entt::null;

		void _AddChild(GameObject child);

	public:
		GameObject() = default;
		GameObject(const GameObject&) = default;
		GameObject(entt::registry* registry, entt::entity entity)
			: mRegistry(registry)
			, mEntity(entity)
		{
		}

		inline bool operator ==(const GameObject& other)
		{
			return mEntity == other.mEntity;
		}

		bool IsValid() const;

		void Destroy();


		inline entt::entity GetEntity() const { return mEntity; }

		bool HasParent() const;

		void SetTag(const std::string& tag);

		template<typename T>
		inline T& AddComponent(T component)
		{
			return mRegistry->emplace<T>(mEntity, component);
		}

		template<typename T, class... Args>
		inline T& AddComponent(Args... args)
		{
			return mRegistry->emplace<T>(mEntity, args...);
		}

		template<typename T>
		inline void RemoveComponent()
		{
			mRegistry->remove<T>(mEntity);
		}

		template<typename T>
		inline T& GetComponent()
		{
			return mRegistry->get<T>(mEntity);
		}

		template<typename T>
		inline const T& GetComponent() const
		{
			return mRegistry->get<T>(mEntity);
		}

		template<typename T>
		inline bool HasComponent() const
		{
			return mRegistry->any_of<T>(mEntity);
		}

		void ForEachChild(std::function<void(GameObject&)> func);

		void ForEachChild(std::function<void(const GameObject&)> func) const;

		void RemoveChildren(std::vector<GameObject> children);

		void SetParent(GameObject parent);

		glm::mat4 GetWorldSpaceTransform() const;
		glm::mat4 GetInterpolatedWorldSpaceTransform(float alpha) const;

		std::tuple<glm::vec4, glm::vec4> GetAABB() const;
	};
}

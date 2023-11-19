#pragma once 

#include "core/Core.h"
#include "GameObject.h"

#include <entt/entt.hpp>

namespace scene
{
	class Scene
	{
	private:
		entt::registry mRegistry;
		std::queue<entt::entity> mDeferredRemovalQueue;

		std::mutex mMutex;

	public:
		Scene();

		GameObject Get(entt::entity e);

		GameObject CreateGameObject(const std::string& name = "GameObject", GameObject parent = GameObject());
		void DestroyGameObject(GameObject obj);

		void FlushDestructionQueue();

		void ForEach(std::function<void(GameObject)>&& func);
		void ForEach(std::function<void(const GameObject)>&& func) const;

		template<class... Filters>
		void ForEach(std::function<void(GameObject)>&& func)
		{
			mRegistry.view<Filters...>().each([&](entt::entity entity, Filters&...)
			{
				func({ &mRegistry, entity });
			});
		}

		template<class... Filters>
		void ForEach(std::function<void(const GameObject)>&& func) const
		{
			mRegistry.view<Filters...>().each([&](const entt::entity entity, const Filters&...)
			{
				func({ &mRegistry, entity });
			});
		}

		// TODO (danielg): there MUST be a faster way to do this
		template<class... Filters>
		u32 Count() const
		{
			u32 result = 0;
			mRegistry.view<Filters...>().each([&result](const entt::entity entity, const Filters&...)
			{
				result++;
			});
			return result;
		}
	};
}
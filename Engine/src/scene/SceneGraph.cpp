#include "SceneGraph.h"

#include "BaseComponents.h"

using namespace scene;


Scene::Scene()
{
}

void Scene::ForEach(std::function<void(GameObject)>&& func)
{
	mRegistry.each([this, &func](auto entity)
	{
		func(GameObject(&mRegistry, entity));
	});
}

GameObject Scene::Get(entt::entity entity)
{
	return GameObject(&mRegistry, entity);
}

GameObject Scene::CreateGameObject(const std::string& name, GameObject parent)
{
	auto entity = mRegistry.create();

	GameObject obj(&mRegistry, entity);
	obj.AddComponent<TransformComponent>();
	obj.AddComponent<TagComponent>().tag = name;

	if (parent.IsValid())
	{
		obj.SetParent(parent);
	}

	return obj;
}

void Scene::DestroyGameObject(GameObject obj)
{
	mDeferredRemovalQueue.push(obj.GetEntity());
}

void Scene::FlushDestructionQueue()
{
	while (!mDeferredRemovalQueue.empty())
	{
		auto entity = mDeferredRemovalQueue.front();
		GameObject(&mRegistry, entity).Destroy();

		mDeferredRemovalQueue.pop();
	}
}
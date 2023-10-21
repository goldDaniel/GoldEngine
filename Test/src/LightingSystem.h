#pragma once 

#include "core/Core.h"
#include "scene/SceneGraph.h"
#include "scene/GameSystem.h"
#include "scene/BaseComponents.h"

class LightingSystem : public scene::GameSystem
{
private: 
	
	scene::GameObject mLightBuffer{};
	u32 mPrevDirectionalLightCount = 0;

	bool CheckSetDirectionalLightsDirty(const scene::Scene& scene)
	{
		u32 dirLightCount = scene.Count<DirectionalLightComponent>();

		if (mPrevDirectionalLightCount != dirLightCount)
		{
			mPrevDirectionalLightCount = dirLightCount;
			return true;
		}

		return false;
	}

public:
	
	virtual void Tick(scene::Scene& scene, float dt)
	{
		if (!mLightBuffer.IsValid())
		{
			mLightBuffer = scene.CreateGameObject();
			mLightBuffer.AddComponent<LightBufferComponent>();
		}

		if (CheckSetDirectionalLightsDirty(scene))
		{	
			LightBufferComponent& buffer = mLightBuffer.GetComponent<LightBufferComponent>();
			buffer.isDirty = true;

			u16 dirCount = 0;
			scene.ForEach<DirectionalLightComponent>([&buffer, &dirCount](const scene::GameObject obj) mutable
			{
				DEBUG_ASSERT(dirCount < LightBufferComponent::MAX_CASTERS, "Will overflow light buffer!");

				const auto& transform = obj.GetComponent<TransformComponent>();
				const auto& light = obj.GetComponent<DirectionalLightComponent>();
				
				//TODO (danielg): shadowmap paging here
				i32 shadowMapIndex = -1;

				buffer.lightBuffer.directionalLights[dirCount++] = LightBufferComponent::DirectionalLight{ glm::normalize(light.direction), light.color, {0,0,0, shadowMapIndex } };
			});
			buffer.lightBuffer.lightCounts.x = dirCount;
		}


	}
};
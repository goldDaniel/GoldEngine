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

	bool CheckSetDirectionalLightsDirty(scene::Scene& scene)
	{
		bool result = false;

		u32 dirLightCount = 0;
		scene.ForEach<DirectionalLightComponent>([&result, &dirLightCount](scene::GameObject& obj)
		{
			auto& light = obj.GetComponent<DirectionalLightComponent>();
			if (light.direction.w > 0)
			{
				result = true;
				light.direction.w = 0;
			}

			dirLightCount++;
		});

		if (mPrevDirectionalLightCount != dirLightCount)
		{
			mPrevDirectionalLightCount = dirLightCount;
			result = true;
		}

		return result;
	}

public:
	
	virtual void Tick(scene::Scene& scene, float dt)
	{
		if (!mLightBuffer.IsValid())
		{
			mLightBuffer = scene.CreateGameObject("Light data buffer");
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

				DEBUG_ASSERT(light.direction.w == 0, "Dirty flag must not be set at this point!");
				buffer.lightBuffer.directionalLights[dirCount++] = LightBufferComponent::DirectionalLight{ glm::normalize(light.direction), light.color, {0,0,0, shadowMapIndex } };
			});
			buffer.lightBuffer.lightCounts.x = dirCount;
		}


	}
};
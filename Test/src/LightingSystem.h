#pragma once 

#include "core/Core.h"
#include "scene/SceneGraph.h"
#include "scene/GameSystem.h"
#include "scene/BaseComponents.h"
 
#include "ShadowMapService.h"

class LightingSystem : public scene::GameSystem
{
private: 
	
	scene::GameObject mLightBuffer{};
	u32 mPrevDirectionalLightCount = 0;
	u32 mPrevPointLightCount = 0;

	bool CheckSetPointLightsDirty(scene::Scene& scene)
	{
		bool result = (mPrevPointLightCount == 0);

		u32 pointLightCount = 0;
		scene.ForEach<PointLightComponent>([&result, &pointLightCount](scene::GameObject obj)
			{
				auto& light = obj.GetComponent<PointLightComponent>();
				//light.color.w marks dirty, since PointLight uses the transform component for position
				if (light.color.w > 0) 
				{
					result = true;
					light.color.w = 0;
				}

				pointLightCount++;
			});

		if (mPrevPointLightCount != pointLightCount)
		{
			mPrevPointLightCount = pointLightCount;
			result = true;
		}

		return result;
	}

	bool CheckSetDirectionalLightsDirty(scene::Scene& scene)
	{
		bool result = (mPrevDirectionalLightCount == 0);
		
		u32 dirLightCount = 0;
		scene.ForEach<DirectionalLightComponent>([&result, &dirLightCount](scene::GameObject obj)
		{
			auto& light = obj.GetComponent<DirectionalLightComponent>();
			if (light.direction.w > 0 || light.color.w > 0)
			{
				result = true;
				light.direction.w = 0;
				light.color.w = 0;
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
		// reset/init on first frame 
		if (!mLightBuffer.IsValid())
		{
			mLightBuffer = scene.CreateGameObject("Light data buffer");
			mLightBuffer.AddComponent<LightBufferComponent>();
			Singletons::Get()->Resolve<ShadowMapService>()->Reset();
		}

		// TODO (danielg): This updates and uploads ALL lights when a light is modified. Only update touched  lights
		if (CheckSetDirectionalLightsDirty(scene))
		{	
			LightBufferComponent& buffer = mLightBuffer.GetComponent<LightBufferComponent>();
			buffer.isDirty = true;

			u16 dirCount = 0;
			scene.ForEach<DirectionalLightComponent>([&buffer, &dirCount](scene::GameObject obj) mutable
			{
				DEBUG_ASSERT(dirCount < LightBufferComponent::MAX_CASTERS, "Will overflow light buffer!");

				const auto& transform = obj.GetComponent<TransformComponent>();
				const auto& light = obj.GetComponent<DirectionalLightComponent>();
				DEBUG_ASSERT(light.direction.w == 0, "Dirty flag must not be set at this point!");

				int shadowMapIndex = -1;
				if (obj.HasComponent<ShadowMapComponent>()) {
					auto& shadowMap = obj.GetComponent<ShadowMapComponent>();
					if (shadowMap.shadowMapIndex[0] == -1)
					{
						shadowMapIndex = Singletons::Get()->Resolve<ShadowMapService>()->GetNextAvailablePageLocation().shadowMapIndex;
						shadowMap.shadowMapIndex[0] = shadowMapIndex;
						shadowMap.dirty = true;
					}
				}

				buffer.lightBuffer.directionalLights[dirCount++] = LightBufferComponent::DirectionalLight{ glm::normalize(light.direction), light.color, {0,0,0, shadowMapIndex } };	
			});
			buffer.lightBuffer.lightCounts.x = dirCount;
		}
		if (CheckSetPointLightsDirty(scene))
		{
			LightBufferComponent& buffer = mLightBuffer.GetComponent<LightBufferComponent>();
			buffer.isDirty = true;

			u16 pointCount = 0;
			scene.ForEach<PointLightComponent>([&, this](scene::GameObject& obj)
			{
				DEBUG_ASSERT(pointCount < LightBufferComponent::MAX_CASTERS, "Will overflow light buffer!");

				const auto& transform = obj.GetComponent<TransformComponent>();
				const auto& light = obj.GetComponent<PointLightComponent>();

				DEBUG_ASSERT(light.color.w == 0, "Dirty flag must not be set at this point!");

				std::array<int, 6> shadowMapIndices{ -1,-1,-1,-1,-1,-1 };
				if (obj.HasComponent<ShadowMapComponent>())
				{
					auto& shadow = obj.GetComponent<ShadowMapComponent>();
					for (auto& index : shadow.shadowMapIndex)
					{
						if (index == -1)
						{
							const auto& page = Singletons::Get()->Resolve<ShadowMapService>()->GetNextAvailablePageLocation();
							index = page.shadowMapIndex;
							shadow.dirty = true;
						}
					}
					shadowMapIndices = shadow.shadowMapIndex;
				}

				buffer.lightBuffer.pointLights[pointCount++] = LightBufferComponent::PointLight
				{
					{ transform.position, 1},
					  light.color,
					{ light.falloff, 0, shadowMapIndices[0], shadowMapIndices[1]},
					{ shadowMapIndices[2], shadowMapIndices[3], shadowMapIndices[4], shadowMapIndices[5]}
				};
			});
			buffer.lightBuffer.lightCounts.y = pointCount;
		}
	}
};
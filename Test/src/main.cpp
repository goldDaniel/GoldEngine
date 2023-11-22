#include "core/Core.h"
#include "core/Application.h"

#include "graphics/Vertex.h"
#include "graphics/FrameEncoder.h"
#include <graphics/MaterialManager.h>


#include "scene/SceneGraph.h"
#include "scene/SceneLoader.h"

#include "ui/LogWindow.h"
#include "ui/SceneWindow.h"
#include "ui/PropertyWindow.h"
#include "ui/ViewportWindow.h"
#include "ui/PerformanceWindow.h"

#include "RenderSystem.h"
#include "LightingSystem.h"
#include "DebugCameraSystem.h"

#include "ShadowMapService.h"


class TestApp : public gold::Application
{
private:
	scene::Scene mScene;
	DebugCameraSystem mCameraSystem;
	LightingSystem mLightingSystem;
	RenderSystem mRenderSystem;


	graphics::FrameBuffer mGBuffer;

	ViewportWindow* mViewport = nullptr;

	bool mFirstFrame = true;

	void ResizeGBuffer(gold::FrameEncoder& encoder)
	{
		
	}

public:
	TestApp(gold::ApplicationConfig&& config)
		: gold::Application(std::move(config))
	{
	}

protected:

	virtual void Init() override
	{
		AddEditorWindow(std::make_unique<LogWindow>());
		AddEditorWindow(std::make_unique<PerformanceWindow>(true));
		auto sceneWindow = AddEditorWindow(std::make_unique<SceneWindow>(&mScene));
		auto propertyWindow = AddEditorWindow(std::make_unique<PropertyWindow>(mScene, [sceneWindow]()
		{
			return sceneWindow->GetSelected();
		}));

		auto camera = mScene.CreateGameObject("Camera");
		camera.AddComponent<DebugCameraComponent>();

		mCameraSystem = DebugCameraSystem(this);

		Singletons::Get()->Register<ShadowMapService>([]() { return std::make_shared<ShadowMapService>(); });
		Singletons::Get()->Register<MaterialManager>([]() { return std::make_shared<MaterialManager>(); });
	}

	virtual void Update(float delta, gold::FrameEncoder& encoder) override
	{
		if(mFirstFrame)
		{
			auto obj = scene::Loader::LoadGameObjectFromModel(mScene, encoder, "sponza2/sponza.gltf");
			obj.GetComponent<TransformComponent>().scale = { 0.05f, 0.05f, 0.05f };

			auto lightObj = mScene.CreateGameObject("Directional Light");
			
			/*auto& light = lightObj.AddComponent<DirectionalLightComponent>();
			light.direction = { 0.05f, -0.7f, 0.25f, 1.0f };
			light.color = { 1,1,1,1 };

			auto& shadow = lightObj.AddComponent<ShadowMapComponent>();
			shadow.left = -90;
			shadow.right = 90;
			shadow.bottom = -90;
			shadow.top = 90;
			shadow.nearPlane = -90;
			shadow.farPlane = 90;
			shadow.dirty = true;*/

			{
				auto lightObj = mScene.CreateGameObject("Red Light");

				auto& transform = lightObj.GetComponent<TransformComponent>();
				auto& light = lightObj.AddComponent<PointLightComponent>();
				auto& shadow = lightObj.AddComponent<ShadowMapComponent>();

				transform.position.x = -20;
				transform.position.y = 40;
				light.color = { 2, 0, 0, 1 };
				light.falloff = 20;

				shadow.PCFSize = 9;
				shadow.aspect = 1;
				shadow.FOV = glm::radians(90.f);
				shadow.nearPlane = 0.5;
				shadow.farPlane = 100;
				shadow.shadowMapBias.fill(0);
			}
			{
				auto lightObj = mScene.CreateGameObject("Green Light");

				auto& transform = lightObj.GetComponent<TransformComponent>();
				auto& light = lightObj.AddComponent<PointLightComponent>();
				auto& shadow = lightObj.AddComponent<ShadowMapComponent>();

				transform.position.x = 0;
				transform.position.y = 40;
				light.color = { 0, 2, 0, 1 };
				light.falloff = 20;

				shadow.PCFSize = 9;
				shadow.aspect = 1;
				shadow.FOV = glm::radians(90.f);
				shadow.nearPlane = 0.5;
				shadow.farPlane = 100;
				shadow.shadowMapBias.fill(0);
			}
			{
				auto lightObj = mScene.CreateGameObject("Blue Light");

				auto& transform = lightObj.GetComponent<TransformComponent>();
				auto& light = lightObj.AddComponent<PointLightComponent>();
				auto& shadow = lightObj.AddComponent<ShadowMapComponent>();

				transform.position.x = 20;
				transform.position.y = 40;
				light.color = { 0, 0, 2, 1 };
				light.falloff = 20;

				shadow.PCFSize = 9;
				shadow.aspect = 1;
				shadow.FOV = glm::radians(90.f);
				shadow.nearPlane = 0.5;
				shadow.farPlane = 100;
				shadow.shadowMapBias.fill(0);
			}

			mFirstFrame = false;
		}

		mCameraSystem.Tick(mScene, delta);

		/*const graphics::FrameBuffer& fb = mRenderSystem.GetRenderTarget();
		mViewport->SetTexture(fb.mTextures[0].idx);*/

		mRenderSystem.SetEncoder(&encoder);

		/*u32 width = mViewport->GetSize().x;
		u32 height = mViewport->GetSize().y;
		if (width == 0 || height == 0)*/
		//{
			u32 width  = GetScreenSize().x;
			u32 height = GetScreenSize().y;
		//}

		mLightingSystem.Tick(mScene, delta);

		mRenderSystem.ResizeGBuffer(width, height);
		mRenderSystem.Tick(mScene, delta);
	}
};


gold::Application* CreateApplication()
{
	gold::ApplicationConfig config;

	config.title = "Test App";
	config.windowWidth = 800;
	config.windowHeight = 600;
	config.maximized = false;

	return new TestApp(std::move(config));
}
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

#include "RenderingToggles.h"

#include "RenderSystem.h"
#include "LightingSystem.h"
#include "DebugCameraSystem.h"

#include "RenderingToggles.h"
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

public:
	TestApp(gold::ApplicationConfig&& config)
		: gold::Application(std::move(config))
	{
	}

protected:

	virtual void Init() override
	{
		AddEditorWindow(std::make_unique<LogWindow>());
		AddEditorWindow(std::make_unique<RenderingTogglesWindow>());
		AddEditorWindow(std::make_unique<PerformanceWindow>(true));
		auto sceneWindow = AddEditorWindow(std::make_unique<SceneWindow>(&mScene));
		AddEditorWindow(std::make_unique<PropertyWindow>(mScene, [sceneWindow]()
		{
			return sceneWindow->GetSelected();
		}));

		auto camera = mScene.CreateGameObject("Camera");
		auto& cam = camera.AddComponent<DebugCameraComponent>();
		cam.mCamera.Far = 500;

		mCameraSystem = DebugCameraSystem(this);

		Singletons::Get()->Register<ShadowMapService>([]() { return std::make_shared<ShadowMapService>(); });
		Singletons::Get()->Register<MaterialManager>([]() { return std::make_shared<MaterialManager>(); });
		Singletons::Get()->Register<RenderingToggles>([]() { return std::make_shared<RenderingToggles>(); });
	}

	virtual void Update(float delta, gold::FrameEncoder& encoder) override
	{
		if(mFirstFrame)
		{
			auto obj = scene::Loader::LoadGameObjectFromModel(mScene, encoder, "sponza2/sponza.gltf");
			obj.GetComponent<TransformComponent>().scale = { 0.125f, 0.125f, 0.125f };
			
			{
				auto lightObj = mScene.CreateGameObject("Red Light");

				auto& transform = lightObj.GetComponent<TransformComponent>();
				auto& light = lightObj.AddComponent<PointLightComponent>();
				auto& shadow = lightObj.AddComponent<ShadowMapComponent>();

				transform.position.x = -120;
				transform.position.y = 20;
				light.color = { 500, 500, 500, 1 };
				light.falloff = 500;

				shadow.PCFSize = 9;
				shadow.perspective.aspect = 1;
				shadow.perspective.FOV = glm::radians(90.f);
				shadow.nearPlane = 2;
				shadow.farPlane = 500;
				shadow.shadowMapBias.fill(0);
			}
			{
				auto lightObj = mScene.CreateGameObject("Green Light");

				auto& transform = lightObj.GetComponent<TransformComponent>();
				auto& light = lightObj.AddComponent<PointLightComponent>();
				auto& shadow = lightObj.AddComponent<ShadowMapComponent>();

				transform.position.x = 0;
				transform.position.y = 70;
				light.color = { 500, 500, 500, 1 };
				light.falloff = 500;

				shadow.PCFSize = 9;
				shadow.perspective.aspect = 1;
				shadow.perspective.FOV = glm::radians(90.f);
				shadow.nearPlane = 2;
				shadow.farPlane = 500;
				shadow.shadowMapBias.fill(0);
			}
			{
				auto lightObj = mScene.CreateGameObject("Blue Light");

				auto& transform = lightObj.GetComponent<TransformComponent>();
				auto& light = lightObj.AddComponent<PointLightComponent>();
				auto& shadow = lightObj.AddComponent<ShadowMapComponent>();

				transform.position.x = 120;
				transform.position.y = 20;
				light.color = { 500, 500, 500, 1 };
				light.falloff = 500;

				shadow.PCFSize = 9;
				shadow.perspective.aspect = 1;
				shadow.perspective.FOV = glm::radians(90.f);
				shadow.nearPlane = 2;
				shadow.farPlane = 500;
				shadow.shadowMapBias.fill(0);
			}

			/*{
				auto lightObj = mScene.CreateGameObject("Directional Light");

				auto& light = lightObj.AddComponent<DirectionalLightComponent>();
				auto& shadow = lightObj.AddComponent<ShadowMapComponent>();

				light.color = { 8, 8, 7, 1 };
				light.direction = { 0.1f, -0.55f, 0.35f, 0.0f };
				shadow.ortho.bottom = -256;
				shadow.ortho.top = 256;
				shadow.ortho.left = -256;
				shadow.ortho.right = 256;
				shadow.nearPlane = -256;
				shadow.farPlane = 256;
				shadow.PCFSize = 2;
				shadow.shadowMapBias.fill(0);
			}*/

			mFirstFrame = false;
		}

		mCameraSystem.Tick(mScene, delta);


		mRenderSystem.SetEncoder(&encoder);
		u32 width = (u32)GetScreenSize().x;
		u32 height = (u32)GetScreenSize().y;
		
		mLightingSystem.Tick(mScene, delta);
		mRenderSystem.ResizeGBuffer(width, height);
		mRenderSystem.Tick(mScene, delta);

		if (Singletons::Get()->Resolve<gold::Input>()->IsKeyJustPressed(KeyCode::r))
		{
			RenderSystem::kReloadShaders = true;
		}
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
#include "core/Core.h"
#include "core/Application.h"

#include "graphics/Vertex.h"
#include "graphics/FrameEncoder.h"

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
		// mViewport = AddEditorWindow(std::make_unique<ViewportWindow>());

		auto camera = mScene.CreateGameObject("Camera");
		camera.AddComponent<DebugCameraComponent>();

		mCameraSystem = DebugCameraSystem(this);
	}

	virtual void Update(float delta, gold::FrameEncoder& encoder) override
	{
		if(mFirstFrame)
		{
			auto obj = scene::Loader::LoadGameObjectFromModel(mScene, encoder, "sponza2/sponza.gltf");
			obj.GetComponent<TransformComponent>().scale = { 0.05f, 0.05f, 0.05f };

			auto lightObj = mScene.CreateGameObject("Directional Light");
			auto& light = lightObj.AddComponent<DirectionalLightComponent>();
			
			light.direction = { 0.1f, -0.8f, 0.1f, 0.0f };
			light.color = { 1,1,1,1 };

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
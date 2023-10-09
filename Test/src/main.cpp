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
#include "DebugCameraSystem.h"



class TestApp : public gold::Application
{
private:
	scene::Loader::Status mStatus = scene::Loader::Status::None;
	scene::Scene mScene;
	DebugCameraSystem mCameraSystem;
	RenderSystem mRenderSystem;
	
	graphics::FrameBuffer mGBuffer;

	ViewportWindow* mViewport = nullptr;

	bool mFirstFrame = true;

	void ResizeGBuffer(gold::FrameEncoder& encoder)
	{
		u32 width = mViewport->GetSize().x;
		u32 height = mViewport->GetSize().y;
		if (width == 0 || height == 0)
		{
			width = GetScreenSize().x;
			height = GetScreenSize().y;
		}

		if (mGBuffer.mHandle.idx == 0 ||
			mGBuffer.mWidth != width || mGBuffer.mHeight != height)
		{
			if (mGBuffer.mHandle.idx)
			{
				encoder.DestroyFrameBuffer(mGBuffer.mHandle);
				mGBuffer.mHandle.idx = 0;
			}

			{
				using namespace graphics;
				FrameBufferDescription fbDesc;

				TextureDescription2D albedoDesc;
				albedoDesc.mWidth = width;
				albedoDesc.mHeight = height;
				albedoDesc.mFormat = TextureFormat::RGB_U8;
				fbDesc.Put(graphics::OutputSlot::Color0, albedoDesc);

				TextureDescription2D normalDesc;
				normalDesc.mWidth = width;
				normalDesc.mHeight = height;
				normalDesc.mFormat = TextureFormat::RGB_FLOAT;
				fbDesc.Put(graphics::OutputSlot::Color1, normalDesc);

				TextureDescription2D coeffDesc;
				coeffDesc.mWidth = width;
				coeffDesc.mHeight = height;
				coeffDesc.mFormat = TextureFormat::RGB_U8;
				fbDesc.Put(graphics::OutputSlot::Color2, coeffDesc);

				TextureDescription2D depthDesc;
				depthDesc.mWidth = width;
				depthDesc.mHeight = height;
				depthDesc.mFormat = TextureFormat::DEPTH;
				fbDesc.Put(graphics::OutputSlot::Depth, depthDesc);

				mGBuffer = encoder.CreateFrameBuffer(fbDesc);
			}
		}
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
		mViewport = AddEditorWindow(std::make_unique<ViewportWindow>());

		auto camera = mScene.CreateGameObject("Camera");
		camera.AddComponent<DebugCameraComponent>();

		mCameraSystem = DebugCameraSystem(this);
	}

	virtual void Update(float delta, gold::FrameEncoder& encoder) override
	{
		mScene.FlushDestructionQueue();
		
		mViewport->SetTexture(mGBuffer.mTextures[0].idx);

		ResizeGBuffer(encoder);

		if (mStatus == scene::Loader::Status::Loading || mStatus == scene::Loader::Status::None)
		{
			mStatus = scene::Loader::LoadGameObjectFromModel(mScene, encoder, "sponza2/sponza.gltf");
		}

		mCameraSystem.Tick(mScene, delta);

		mRenderSystem.SetEncoder(&encoder);
		mRenderSystem.SetRenderTarget(mGBuffer);
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
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

#include "RenderSystem.h"
#include "DebugCameraSystem.h"



class TestApp : public gold::Application
{
private:
	scene::Loader::Status mStatus = scene::Loader::Status::None;
	scene::Scene mScene;
	DebugCameraSystem mCameraSystem;
	RenderSystem mRenderSystem;
	
	graphics::FrameBuffer mGameBuffer;
	
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
		auto sceneWindow = AddEditorWindow(std::make_unique<SceneWindow>(&mScene));
		auto propertyWindow = AddEditorWindow(std::make_unique<PropertyWindow>(mScene, [sceneWindow]()
		{
			return sceneWindow->GetSelected();
		}));
		mViewport = AddEditorWindow(std::make_unique<ViewportWindow>());

		auto camera = mScene.CreateGameObject("Camera");
		camera.AddComponent<DebugCameraComponent>();
	}

	virtual void Update(float delta, gold::FrameEncoder& encoder) override
	{
		mScene.FlushDestructionQueue();

		u32 width = mViewport->GetSize().x;
		u32 height = mViewport->GetSize().y;
		if (width == 0 || height == 0)
		{
			width = GetScreenSize().x;
			height = GetScreenSize().y;
		}

		if (mGameBuffer.mHandle.idx == 0 ||
			mGameBuffer.mWidth != width || mGameBuffer.mHeight != height)
		{
			graphics::FrameBufferDescription desc;

			{
				graphics::TextureDescription2D texDesc;
				texDesc.mWidth = width;
				texDesc.mHeight = height;
				texDesc.mFormat = graphics::TextureFormat::RGB_U8;
				desc.Put(graphics::OutputSlot::Color0, texDesc);
			}

			{
				graphics::TextureDescription2D depthDesc;
				depthDesc.mWidth = width;
				depthDesc.mHeight = height;
				depthDesc.mFormat = graphics::TextureFormat::DEPTH;
				desc.Put(graphics::OutputSlot::Depth, depthDesc);
			}

			if (mGameBuffer.mHandle.idx)
			{
				encoder.DestroyFrameBuffer(mGameBuffer.mHandle);
			}

			mGameBuffer = encoder.CreateFrameBuffer(desc);
		}

		if (mStatus == scene::Loader::Status::Loading || mStatus == scene::Loader::Status::None)
		{
			mStatus = scene::Loader::LoadGameObjectFromModel(mScene, encoder, "sponza2/sponza.gltf");
		}

		mViewport->SetTexture(mGameBuffer.mTextures[0].idx);

		mCameraSystem.Tick(mScene, delta);

		mRenderSystem.SetEncoder(&encoder);
		mRenderSystem.SetRenderTarget(mGameBuffer);
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
#include "core/Core.h"
#include "core/Application.h"

#include "graphics/Vertex.h"
#include "graphics/FrameEncoder.h"

#include "scene/SceneGraph.h"

#include "ui/LogWindow.h"
#include "ui/ViewportWindow.h"

#include "RenderSystem.h"

class TestApp : public gold::Application
{
private:
	scene::Scene mScene;
	RenderSystem mRenderSystem;
	
	graphics::FrameBuffer mGameBuffer;
	
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
	}

	virtual void Update(float delta, gold::FrameEncoder& encoder) override
	{
		if (mFirstFrame)
		{
			mFirstFrame = false;
		}

		mRenderSystem.SetScreenSize(GetScreenSize());
		mRenderSystem.SetEncoder(&encoder);
		//mRenderSystem.SetRenderTarget(mGameBuffer);
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
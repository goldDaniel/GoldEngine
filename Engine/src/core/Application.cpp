#include "Application.h"

#include "memory/LinearAllocator.h"
#include "graphics/FrameDecoder.h"
#include "graphics/FrameEncoder.h"
#include "graphics/RenderCommands.h"

using namespace gold;

void Application::Run()
{
	Init();

	mRunning = true;

	// virtual command buffer size
	u64 size = 8 * 1024 * 1024;
	mEncoders.Init([this, size]() { return std::make_unique<FrameEncoder>(mRenderResources, size); });

	// frame allocator size 
	size = 4 * 1024 * 1024;
	mFrameAllocator = std::make_unique<LinearAllocator>(malloc(size), size);

	std::thread updateThread = std::thread(&Application::UpdateThread, this);

	RenderThread(); 

	// assures we don't wait on a condition that wont get signaled
	{
		std::unique_lock lock(mSwapMutex);
		mSwapCond.notify_all();
	}
	
	updateThread.join();
	G_ENGINE_INFO("Threads terminated, shutting down.");
}

void Application::UpdateThread()
{
	G_ENGINE_WARN("Update thread starting...");

	f32 step = 1.0f / 30.f;
	uint32_t prevTime = mPlatform->GetElapsedTimeMS();
	while (mRunning)
	{
		uint32_t currTime = mPlatform->GetElapsedTimeMS();
		f32 frameTime = static_cast<float>(currTime - prevTime) / 1000.f;
		prevTime = currTime;

		mEncoders.Get()->Begin();
		Update(frameTime, *mEncoders.Get());
		mEncoders.Get()->End();

		mTime += frameTime;
		{
			std::unique_lock lock(mSwapMutex);
			mUpdateComplete = true;
			mSwapCond.notify_one();
			mSwapCond.wait(lock, [this] { return !mUpdateComplete || !mRunning; });
		}
	}

	G_ENGINE_WARN("Update thread shutting down...");
}

void Application::RenderThread()
{
	G_ENGINE_WARN("Render thread starting...");

	mRenderer = std::unique_ptr<graphics::Renderer>();
	mRenderer->Init(mPlatform->GetWindowHandle());

	while (mRunning)
	{
		gold::FrameEncoder* mReadEncoder = nullptr;
		{
			std::unique_lock lock(mSwapMutex);
			mSwapCond.wait(lock, [this] { return mUpdateComplete || !mRunning; });
			
			mPlatform->PlatformEvents(*this);
			mUpdateComplete = false;
			mReadEncoder = mEncoders.Get().get();
			mEncoders.Swap();
			mSwapCond.notify_one();
		}
		if (!mRunning) break;

		mFrameAllocator->Reset();
		BinaryReader reader = mReadEncoder->GetReader();

		
		mRenderer->SetBackBufferSize((int)mConfig.windowWidth, (int)mConfig.windowHeight);
		mRenderer->ClearBackBuffer();

		mRenderer->BeginFrame();
		if(reader.HasData())
		{
			gold::FrameDecoder::Decode(*mRenderer, *mFrameAllocator, mRenderResources, reader);
		}
		mUI.OnImguiRender(*mRenderer, mRenderResources);
		mRenderer->EndFrame();
	}

	G_ENGINE_WARN("Render thread shutting down...");
}

Application::Application(ApplicationConfig&& config)
	: mConfig(std::move(config))
	, mTime(0)
	, mRunning(false)
{

}

Application::~Application()
{
	mFrameAllocator->Free();
	mRunning = false;
}
 
void Application::StartApplication(std::unique_ptr<Platform> platform)
{
	mPlatform = std::move(platform);
	mPlatform->InitializeWindow(mConfig);
}

void Application::Shutdown()
{
	mRunning = false;
}

float Application::GetTime() const
{
	return mTime;
}

const std::vector<std::string>& Application::GetCommandArgs() const
{
	return mPlatform->GetCommandArgs();
}

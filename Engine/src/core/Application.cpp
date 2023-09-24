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

	mEncoders.Get() = std::make_unique<FrameEncoder>(mRenderResources);
	mEncoders.Swap();
	mEncoders.Get() = std::make_unique<FrameEncoder>(mRenderResources);

	// virtual command buffer size
	u64 size = 128 * 1024 * 1024;
	mFrameAllocator = std::make_unique<LinearAllocator>(malloc(size), size);

	std::thread updateThread = std::thread(&Application::UpdateThread, this);
	std::thread renderThread = std::thread(&Application::RenderThread, this);

	while (mRunning)
	{
		mPlatform->PlatformEvents(*this);
	}

	// assures we don't wait on a condition that wont get signaled
	{
		std::unique_lock lock(mSwapMutex);
		mSwapCond.notify_all();
	}
	
	updateThread.join();
	renderThread.join();
}

void Application::UpdateThread()
{
	f32 step = 1.0f / 30.f;
	uint32_t prevTime = mPlatform->GetElapsedTimeMS();
	while (mRunning)
	{
		if (!mRunning) break;

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
}

void Application::RenderThread()
{
	mRenderer = std::unique_ptr<graphics::Renderer>();
	mRenderer->Init(mPlatform->GetWindowHandle());

	while (mRunning)
	{
		mFrameAllocator->Reset();

		gold::FrameEncoder* mReadEncoder = nullptr;
		{
			std::unique_lock lock(mSwapMutex);
			mSwapCond.wait(lock, [this] { return mUpdateComplete || !mRunning; });
			mUpdateComplete = false;
			mReadEncoder = mEncoders.Get().get();
			mEncoders.Swap();
			mSwapCond.notify_one();
		}

		BinaryReader reader = mReadEncoder->GetReader();

		mRenderer->SetBackBufferSize((int)mConfig.windowWidth, (int)mConfig.windowHeight);
		mRenderer->BeginFrame();
		while (reader.HasData())
		{
			gold::RenderCommand command = reader.Read<gold::RenderCommand>();
			gold::FrameDecoder::Decode(*mRenderer, *mFrameAllocator, mRenderResources, reader, command);
			if (command == gold::RenderCommand::END) break;
		}
		mRenderer->EndFrame();
	}
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

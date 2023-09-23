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

	u64 size = 128 * 1024 * 1024;
	mFrameAllocator = std::make_unique<LinearAllocator>(malloc(size), size);

	mUpdateThread = std::thread([this]()
	{
		float step = 1.0f / 30.f;
		uint32_t prevTime = mPlatform->GetElapsedTimeMS();
		while (mRunning)
		{
			uint32_t currTime = mPlatform->GetElapsedTimeMS();
			float frameTime = static_cast<float>(currTime - prevTime) / 1000.f;
			prevTime = currTime;

			mAccumulator += frameTime;

			
			std::unique_lock lock(mUpdateMutex);
			mUpdateCond.wait(lock, [this] {return mCanUpdate; });
			
			if (!mRunning) break;

			Update(frameTime, *mEncoders.Get());
			mTime += frameTime;
			
			mCanUpdate = false;

			// signal render
			{
				std::unique_lock lock(mRenderMutex);
				mReadEncoder = mEncoders.Get().get();
				mEncoders.Swap();
				mCanRender = true;
				mRenderCond.notify_one();
			}
		}
	});

	RenderThread();

	// assures we dont wait on a condition that wont get signaled
	{
		std::unique_lock lock(mUpdateMutex);
		mCanUpdate = true;
		mUpdateCond.notify_one();
	}
	mUpdateThread.join();
}

void Application::RenderThread()
{
	mRenderer = std::unique_ptr<graphics::Renderer>();
	mRenderer->Init(mPlatform->GetWindowHandle());

	while (mRunning)
	{
		mPlatform->PlatformEvents(*this);
		if (!mRunning) break;

		// wait for update	
		std::unique_lock lock(mRenderMutex);
		mRenderCond.wait(lock, [this] {return mCanRender; });

		mFrameAllocator->Reset();

		if (mReadEncoder)
		{
			BinaryReader reader = mReadEncoder->GetReader();
			mRenderer->BeginFrame();
			mRenderer->SetBackBufferSize((int)mConfig.windowWidth, (int)mConfig.windowHeight);

			while (reader.HasData())
			{
				gold::RenderCommand command = reader.Read<gold::RenderCommand>();
				gold::FrameDecoder::Decode(*mRenderer, *mFrameAllocator, mRenderResources, reader, command);
				if (command == gold::RenderCommand::END) break;
			}
			mRenderer->EndFrame();

			mReadEncoder = nullptr;
		}

		mCanRender = false;

		{
			std::unique_lock lock(mUpdateMutex);
			mCanUpdate = true;
			mUpdateCond.notify_one();
		}
	}
}

Application::Application(ApplicationConfig&& config)
	: mConfig(std::move(config))
	, mTime(0)
	, mAccumulator(0)
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

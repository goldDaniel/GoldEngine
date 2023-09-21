#include "Application.h"

using namespace gold;

void Application::Run()
{
	Init();

	mRunning = true;
	mRenderThread = std::move(std::thread(&Application::RenderThread, this));

	uint32_t prevTime = mPlatform->GetElapsedTimeMS();
	float step = 1.0f / 30.f;

	while (mRunning)
	{
		std::scoped_lock(mRenderMutex);

		uint32_t currTime = mPlatform->GetElapsedTimeMS();
		float frameTime = static_cast<float>(currTime - prevTime) / 1000.f;
		prevTime = currTime;

		mAccumulator += frameTime;

		mPlatform->PlatformEvents(*this);
		if (!mRunning) break;

		Update(frameTime);
		mTime += frameTime;


		mRenderCond.notify_one();
	}
}

void Application::RenderThread()
{
	mRenderer = std::unique_ptr<graphics::Renderer>();
	mRenderer->Init(mPlatform->GetWindowHandle());

	while (mRunning)
	{
		std::unique_lock<std::mutex> lock(mRenderMutex);
		mRenderCond.wait(lock, [] { return true; });

		Render(*mRenderer);

		lock.unlock();
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
	mRunning = false;
	mRenderThread.join();
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

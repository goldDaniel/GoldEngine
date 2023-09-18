#include "Application.h"

using namespace gold;

Application::Application(ApplicationConfig&& config)
	: mConfig(std::move(config))
	, mTime(0)
	, mAccumulator(0)
	, mRunning(false)
{

}

Application::~Application()
{

}
 
void Application::StartApplication(std::unique_ptr<Platform> platform)
{
	mPlatform = std::move(platform);
	mPlatform->InitializeWindow(mConfig);
	
	mRenderer = std::unique_ptr<graphics::Renderer>();
	mRenderer->Init(mPlatform->GetWindowHandle());
}

void Application::Run()
{
	Init();

	mRunning = true;
	uint32_t prevTime = mPlatform->GetElapsedTimeMS();
	float step = 1.0f / 30.f;

	while (mRunning)
	{
		uint32_t currTime = mPlatform->GetElapsedTimeMS();
		float frameTime = static_cast<float>(currTime - prevTime) / 1000.f;
		prevTime = currTime;

		mAccumulator += frameTime;

		mPlatform->PlatformEvents(*this);
		if (!mRunning) break;

		Update(frameTime);
		mTime += frameTime;
	}
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

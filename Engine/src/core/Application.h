#pragma once

#include "Core.h"
#include "platform/Platform.h"

namespace gold
{
	struct ApplicationConfig
	{
		std::string title;

		u32 windowWidth;
		u32 windowHeight;

		bool maximized;
	};


	class Application
	{
	private:
		std::unique_ptr<Platform> mPlatform = nullptr;

		f32 mTime;
		f32 mAccumulator;

		bool mRunning;

	protected:
		ApplicationConfig mConfig;

	public:
		Application(ApplicationConfig&& config);
		virtual ~Application();

		void StartApplication(std::unique_ptr<Platform> platform);

		void Run();
		void Shutdown();

		float GetTime() const;

		const std::vector<std::string>& GetCommandArgs() const;

		glm::vec2 GetScreenSize() const { return { mConfig.windowWidth, mConfig.windowHeight }; }

		const std::string& GetTitle() const { return mConfig.title; }


	protected:
		virtual void Init() = 0;
		virtual void Update(float delta) = 0;
	};
}

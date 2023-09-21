#pragma once

#include "Core.h"
#include "platform/Platform.h"
#include "graphics/Renderer.h"

#include <thread>
#include <condition_variable>

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
		std::unique_ptr<graphics::Renderer> mRenderer = nullptr;

		std::thread mRenderThread;
		std::mutex mRenderMutex;
		std::condition_variable mRenderCond;
		//RenderCommands commands;


		f32 mTime;
		f32 mAccumulator;

		bool mRunning;

		void RenderThread();

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

		void SetScreenSize(int w, int h) 
		{ 
			mConfig.windowWidth = w; 
			mConfig.windowHeight = h; 
		}

		glm::vec2 GetScreenSize() const 
		{ 
			return { mConfig.windowWidth, mConfig.windowHeight }; 
		}

		const std::string& GetTitle() const { return mConfig.title; }


	protected:
		virtual void Init() = 0;
		virtual void Update(float delta) = 0;
		virtual void Render(graphics::Renderer& renderer) = 0;
	};
}

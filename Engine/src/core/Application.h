#pragma once

#include "Core.h"
#include "platform/Platform.h"
#include "graphics/Renderer.h"
#include "graphics/RenderResources.h"

#include "memory/DoubleBuffered.h"

#include "ui/EditorUI.h"

#include <thread>
#include <condition_variable>

namespace gold
{
	class LinearAllocator;
	class FrameEncoder;
	class BinaryReader;
	class RenderResources;
	
	struct ApplicationConfig
	{
		std::string title;

		u32 windowWidth{};
		u32 windowHeight{};

		bool maximized{};
	};


	class Application
	{
	private:
		std::unique_ptr<Platform> mPlatform;
		

		gold::RenderResources mRenderResources;
		std::unique_ptr<graphics::Renderer> mRenderer;

		gold::DoubleBuffered<std::unique_ptr<gold::LinearAllocator>> mFrameAllocators;
		gold::DoubleBuffered<std::unique_ptr<gold::FrameEncoder>> mEncoders;

		bool mUpdateComplete = false;
		std::mutex mSwapMutex;
		std::condition_variable mSwapCond;

		EditorUI mUI;

		f32 mTime;
		
		bool mRunning;

		void UpdateThread();
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

		template<typename T>
		T* AddEditorWindow(std::unique_ptr<T>&& window)
		{
			T* result = window.get();
			mUI.AddEditorWindow(std::move(window));
			return result;
		}

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
		virtual void Update(float delta, gold::FrameEncoder& encoder) = 0;
	};
}

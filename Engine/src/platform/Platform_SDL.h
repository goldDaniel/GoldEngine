#pragma once

#include "Platform.h"

#include "core/Application.h"
#include "memory/FreeListAllocator.h"

struct SDL_Window;

namespace gold
{
	class Platform_SDL : public Platform
	{
	private:
		SDL_Window* mWindow = nullptr;
	public:
		Platform_SDL(std::vector<std::string>&& commandArgs);

		virtual ~Platform_SDL() override;

		virtual void InitializeWindow(const ApplicationConfig& config) override;

		virtual u32 GetElapsedTimeMS() const override;

		virtual void PlatformEvents(Application& app) override;

		virtual void* GetWindowHandle() const override;
	};
}

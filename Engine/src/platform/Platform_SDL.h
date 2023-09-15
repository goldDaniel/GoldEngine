#pragma once


#include "Platform.h"
#include "core/Application.h"

#include <SDL.h>

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

		virtual uint32_t GetElapsedTimeMS() const override;

		virtual void PlatformEvents(Application& app) override;
	};
}

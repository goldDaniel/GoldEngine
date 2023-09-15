#pragma once 

#include "core/Core.h"

namespace gold
{
	class Application;
	struct ApplicationConfig;

	class Platform
	{
	protected:
		const std::vector<std::string> mCommandArgs;

	public:
		
		Platform(std::vector<std::string>&& commandArgs)
			: mCommandArgs(std::move(commandArgs))
		{
		}

		virtual ~Platform() {}

		virtual void InitializeWindow(const ApplicationConfig& config) = 0;
		virtual void PlatformEvents(Application& app) = 0;

		virtual uint32_t GetElapsedTimeMS() const = 0;

		const std::vector<std::string>& GetCommandArgs() const { return mCommandArgs; }
	};
}
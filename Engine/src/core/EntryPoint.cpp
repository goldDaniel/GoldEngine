#pragma once

#include "core/Core.h"
#include "Application.h"

#include "platform/Platform_SDL.h"


extern gold::Application* CreateApplication();

static std::unique_ptr<gold::Platform> CreatePlatform(std::vector<std::string>&& commandArgs)
{
	return std::move(std::make_unique<gold::Platform_SDL>(std::move(commandArgs)));
}

int main(int argc, char* argv[])
{
	std::vector<std::string> commandArgs;
	for (int i = 0; i < argc; ++i)
	{
		commandArgs.push_back(std::string(argv[i]));
	}

	auto platform = CreatePlatform(std::move(commandArgs));
	auto app = std::shared_ptr<gold::Application>(CreateApplication());

	app->StartApplication(std::move(platform));
	app->Run();

	return 0;
}


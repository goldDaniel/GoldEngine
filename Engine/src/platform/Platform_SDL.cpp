#include "Platform_SDL.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined (__linux__)

#else 
static_assert(false && "Known Platform for SDL backend!");
#endif

#include "core/Application.h"
#include "core/Singletons.h"
#include "core/Input.h"
#include "thirdparty/imgui_impl_sdl2.h"

#include <SDL.h>
#include <filesystem>

#include <imgui.h>



using namespace gold;

static void SetWorkingDirectory(const std::string& pathOffset)
{

#if defined(_WIN32)
	const char* currentDir = SDL_GetBasePath();
	const auto path = std::filesystem::absolute(std::filesystem::path(currentDir + pathOffset));
	const std::string pathAsString = path.string();

	if (!SetCurrentDirectory(pathAsString.c_str()))
	{
		G_ENGINE_ERROR("Failed to set data directory to: {}", pathAsString.c_str());
		DEBUG_ASSERT(false, "Failed to set data directory");
	}
	else
	{
		G_ENGINE_INFO("Working directory set to: {}", pathAsString.c_str());
	}
#elif defined (__linux__)
	G_ENGINE_ERROR("Failed to set data directory to: {}", desiredDir);
	DEBUG_ASSERT(false, "Failed to set data directory");
#else 
	STATIC_ASSERT(false, "Known Platform for SDL backend!");
#endif
}

Platform_SDL::Platform_SDL(std::vector<std::string>&& commandArgs)
	: Platform(std::move(commandArgs))
{	
	const std::string dataRedirectKey = "DataFolderRedirect=";

	for (const std::string& arg : GetCommandArgs())
	{
		u64 idx = arg.find(dataRedirectKey);
		if (idx != std::string::npos)
		{
			std::string path = arg.substr(idx + dataRedirectKey.size(), arg.size());
			SetWorkingDirectory(path);
		}
	}
}

Platform_SDL::~Platform_SDL()
{
	SDL_DestroyWindow(mWindow);
	SDL_Quit();
}

void Platform_SDL::InitializeWindow(const ApplicationConfig& config)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to Initialize Window!", SDL_GetError(), 0);
		SDL_Log("SDL2 video subsystem couldn't be initialized. Error: %s", SDL_GetError());
		exit(1);
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	mWindow = SDL_CreateWindow(config.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, config.windowWidth, config.windowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	SDL_SetWindowResizable(mWindow, (SDL_bool)true);

	if (config.maximized)
	{
		SDL_MaximizeWindow(mWindow);
	}
}

u32 Platform_SDL::GetElapsedTimeMS() const
{
	return SDL_GetTicks();
}

void Platform_SDL::PlatformEvents(Application& app)
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		auto& io = ImGui::GetIO();
		ImGui_ImplSDL2_ProcessEvent(&event);

		switch (event.type)
		{
		case SDL_QUIT:
		{
			G_ENGINE_WARN("Shutting Down.");
			app.Shutdown();
			break;
		}
		case SDL_KEYDOWN:
			if (!io.WantCaptureKeyboard)
			{
				Singletons::Get()->Resolve<Input>()->SetKeyState(static_cast<KeyCode>(event.key.keysym.sym), true);
			}
			break;
		case SDL_KEYUP:
			if (!io.WantCaptureKeyboard)
			{
				Singletons::Get()->Resolve<Input>()->SetKeyState(static_cast<KeyCode>(event.key.keysym.sym), false);
			}
			break;
		case SDL_MOUSEMOTION:
			//if (!io.WantCaptureMouse)
			{
				int x;
				int y;
				SDL_GetMouseState(&x, &y);
				Singletons::Get()->Resolve<Input>()->SetMousePosition(x, y);
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			//if (!io.WantCaptureMouse)
			{
				Singletons::Get()->Resolve<Input>()->SetMouseState(static_cast<MouseButton>(event.button.button), true);
			}
			break;

		case SDL_MOUSEBUTTONUP:
			//if (!io.WantCaptureMouse)
			{
				Singletons::Get()->Resolve<Input>()->SetMouseState(static_cast<MouseButton>(event.button.button), false);
			}
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event)
			{
			case SDL_WINDOWEVENT_SIZE_CHANGED:
			case SDL_WINDOWEVENT_RESIZED:
				app.SetScreenSize(event.window.data1, event.window.data2);
				break;
			}
		}
	}
}

void* Platform_SDL::GetWindowHandle() const
{
	return mWindow;
}
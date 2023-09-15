#include "core/Core.h"
#include "core/Application.h"


class TestApp : public gold::Application
{
public:
	TestApp(gold::ApplicationConfig&& config)
		: gold::Application(std::move(config))
	{
	}

protected:
	virtual void Init() override
	{

	}

	virtual void Update(float delta) override
	{

	}
};


gold::Application* CreateApplication()
{
	gold::ApplicationConfig config;

	config.title = "Test App";
	config.windowWidth = 800;
	config.windowHeight = 600;
	config.maximized = false;

	return new TestApp(std::move(config));
}
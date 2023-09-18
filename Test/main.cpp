#include "core/Core.h"
#include "core/Application.h"


class TestApp : public gold::Application
{
private:
	

	graphics::UniformBuffer mView;

public:
	TestApp(gold::ApplicationConfig&& config)
		: gold::Application(std::move(config))
	{
	}

protected:
	virtual void Init() override
	{
		glm::mat4 mvp;

		glm::vec2 screenSize = GetScreenSize();

		mvp = glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0,0,0 }, glm::vec3{ 0,1,0 });
		mvp *= glm::perspective(glm::radians(65.f), screenSize.x / screenSize.y, 1.f, 100.f);

		mView = mRenderer->CreateUniformBlock(mvp);
	}


	virtual void Update(float delta) override
	{
		mRenderer->BeginFrame();

		mRenderer->EndFrame();
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
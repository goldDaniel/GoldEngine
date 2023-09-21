#include "core/Core.h"
#include "core/Application.h"

#include "graphics/Vertex.h"

class TestApp : public gold::Application
{
private:
	graphics::UniformBuffer mView{};
	graphics::Shader mShader{};

	graphics::MeshHandle mMesh{};
	
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

		mvp = glm::perspective(glm::radians(65.f), screenSize.x / screenSize.y, 1.f, 100.f);
		mvp *= glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0,0,0 }, glm::vec3{ 0,1,0 });

		mView = mRenderer->CreateUniformBlock(mvp);

		std::string vertSrc = util::LoadStringFromFile("shaders/default.vert.glsl");

		std::string fragSrc = util::LoadStringFromFile("shaders/default.frag.glsl");
		mShader = mRenderer->CreateShader(vertSrc.c_str(), fragSrc.c_str());

		
		graphics::VertexLayout layout;
		layout.Push<graphics::VertexLayout::Position3>();
		layout.Push<graphics::VertexLayout::Color3>();

		graphics::VertexBuffer buffer(std::move(layout));

		glm::vec3 pos0 = {-1, -1, 0 };
		glm::vec3 col0 = { 1, 0, 0 };

		glm::vec3 pos1 = { 1, -1, 0 };
		glm::vec3 col1 = { 0, 1, 0 };

		glm::vec3 pos2 = { 0, 1, 0 };
		glm::vec3 col2 = { 0, 0, 1 };

		buffer.Emplace(pos0, col0);
		buffer.Emplace(pos1, col1);
		buffer.Emplace(pos2, col2);

		std::vector<uint16_t> indices =
		{
			0, 1, 2
		};

		graphics::MeshDescription mesh;
		mesh.mIndicesFormat = graphics::IndexFormat::U16;
		mesh.mIndices = mRenderer->CreateIndexBuffer(indices);
		mesh.mIndexCount = 3;

		mesh.mVertexCount = 3;
		mesh.mStride = buffer.GetLayout().Size();
		mesh.offsets.mPositionOffset = buffer.GetLayout().Resolve<graphics::VertexLayout::Position3>().GetOffset();
		mesh.offsets.mColorsOffset= buffer.GetLayout().Resolve<graphics::VertexLayout::Color3>().GetOffset();

		mesh.mInterlacedBuffer = mRenderer->CreateVertexBuffer(buffer.Raw(), buffer.SizeInBytes());

		mMesh = mRenderer->CreateMesh(mesh);
	}


	virtual void Update(float delta) override
	{
		glm::vec2 screenSize = GetScreenSize();

		glm::mat4 mvp = glm::perspective(glm::radians(65.f), screenSize.x / screenSize.y, 1.f, 100.f);
		mvp *= glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0,0,0 }, glm::vec3{ 0,1,0 });
		mRenderer->UpdateUniformBlock(mvp, mView);

		mRenderer->SetBackBufferSize((int)screenSize.x, (int)screenSize.y);
		mRenderer->BeginFrame();

		uint8_t pass = mRenderer->AddRenderPass("Default", graphics::ClearColor::YES, graphics::ClearDepth::YES);
		graphics::RenderState state;
		state.mRenderPass = pass;
		state.mShader = &mShader;
		state.SetUniformBlock("View_UBO", mView);

		mRenderer->DrawMesh(mMesh, state);

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
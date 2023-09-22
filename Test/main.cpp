#include "core/Core.h"
#include "core/Application.h"

#include "graphics/Vertex.h"
#include "graphics/FrameEncoder.h"

class TestApp : public gold::Application
{
private:
	graphics::UniformBufferHandle mView{};
	graphics::ShaderHandle mShader{};

	graphics::MeshHandle mMesh{};
	
	bool mFirstFrame = true;

	gold::RenderResources mRenderResources;

	std::array<std::unique_ptr<gold::FrameEncoder>,2> mEncoders;
	u8 writeEncoder = 0;
	u8 readEncoder = 1;

public:
	TestApp(gold::ApplicationConfig&& config)
		: gold::Application(std::move(config))
	{
	}

protected:

	void InitRenderData()
	{
		glm::mat4 mvp;

		glm::vec2 screenSize = GetScreenSize();

		mvp = glm::perspective(glm::radians(65.f), screenSize.x / screenSize.y, 1.f, 100.f);
		mvp *= glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0,0,0 }, glm::vec3{ 0,1,0 });

		mView = mEncoders[writeEncoder]->CreateUniformBuffer(&mvp, sizeof(glm::mat4));

		std::string vertSrc = util::LoadStringFromFile("shaders/default.vert.glsl");

		std::string fragSrc = util::LoadStringFromFile("shaders/default.frag.glsl");
		mShader = mEncoders[writeEncoder]->CreateShader(vertSrc.c_str(), fragSrc.c_str());


		graphics::VertexLayout layout;
		layout.Push<graphics::VertexLayout::Position3>();
		layout.Push<graphics::VertexLayout::Color3>();

		graphics::VertexBuffer buffer(std::move(layout));

		glm::vec3 pos0 = { -1, -1, 0 };
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
		mesh.mIndices = mEncoders[writeEncoder]->CreateIndexBuffer(indices.data(), sizeof(uint16_t) * indices.size());
		mesh.mIndexCount = 3;

		mesh.mVertexCount = 3;
		mesh.mStride = buffer.GetLayout().Size();
		mesh.offsets.mPositionOffset = buffer.GetLayout().Resolve<graphics::VertexLayout::Position3>().GetOffset();
		mesh.offsets.mColorsOffset = buffer.GetLayout().Resolve<graphics::VertexLayout::Color3>().GetOffset();

		mesh.mInterlacedBuffer = mEncoders[writeEncoder]->CreateVertexBuffer(buffer.Raw(), buffer.SizeInBytes());

		mMesh = mEncoders[writeEncoder]->CreateMesh(mesh);

		std::swap(writeEncoder, readEncoder);
	}

	virtual void Init() override
	{
		mEncoders[0] = std::make_unique<gold::FrameEncoder>(mRenderResources);
		mEncoders[1] = std::make_unique<gold::FrameEncoder>(mRenderResources);
	}


	virtual void Update(float delta) override
	{
		if (mFirstFrame)
		{
			InitRenderData();
			mFirstFrame = false;
		}
	}

	virtual void Render(graphics::Renderer& renderer) override
	{

		/*glm::vec2 screenSize = GetScreenSize();

		glm::mat4 mvp = glm::perspective(glm::radians(65.f), screenSize.x / screenSize.y, 1.f, 100.f);
		mvp *= glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0,0,0 }, glm::vec3{ 0,1,0 });
		renderer.UpdateUniformBlock(mvp, mView);

		renderer.SetBackBufferSize((int)screenSize.x, (int)screenSize.y);
		renderer.BeginFrame();

		uint8_t pass = renderer.AddRenderPass("Default", graphics::ClearColor::YES, graphics::ClearDepth::YES);
		graphics::RenderState state;
		state.mRenderPass = pass;
		state.mShader = mShader;
		state.SetUniformBlock("View_UBO", { mView });

		renderer.DrawMesh(mMesh, state);

		renderer.EndFrame();*/
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
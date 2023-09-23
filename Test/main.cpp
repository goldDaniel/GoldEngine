#include "core/Core.h"
#include "core/Application.h"

#include "memory/LinearAllocator.h"

#include "graphics/Vertex.h"
#include "graphics/FrameEncoder.h"
#include "graphics/RenderCommands.h"

class TestApp : public gold::Application
{
private:
	graphics::UniformBufferHandle mView{};
	graphics::ShaderHandle mShader{};

	graphics::MeshHandle mMesh{};
	
	bool mFirstFrame = true;

	gold::RenderResources mRenderResources;

	std::unique_ptr<gold::FrameEncoder> mReadEncoder;
	std::unique_ptr<gold::FrameEncoder> mWriteEncoder;

	std::unique_ptr<gold::LinearAllocator> mFrameAllocator;

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

		mView = mWriteEncoder->CreateUniformBuffer(&mvp, sizeof(glm::mat4));

		std::string vertSrc = util::LoadStringFromFile("shaders/default.vert.glsl");

		std::string fragSrc = util::LoadStringFromFile("shaders/default.frag.glsl");
		mShader = mWriteEncoder->CreateShader(vertSrc.c_str(), fragSrc.c_str());


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
		mesh.mIndices = mWriteEncoder->CreateIndexBuffer(indices.data(), sizeof(uint16_t) * indices.size());
		mesh.mIndexCount = 3;

		mesh.mVertexCount = 3;
		mesh.mStride = buffer.GetLayout().Size();
		mesh.offsets.mPositionOffset = buffer.GetLayout().Resolve<graphics::VertexLayout::Position3>().GetOffset();
		mesh.offsets.mColorsOffset = buffer.GetLayout().Resolve<graphics::VertexLayout::Color3>().GetOffset();

		mesh.mInterlacedBuffer = mWriteEncoder->CreateVertexBuffer(buffer.Raw(), buffer.SizeInBytes());

		mMesh = mWriteEncoder->CreateMesh(mesh);

		std::swap(mWriteEncoder, mReadEncoder);
	}

	virtual void Init() override
	{
		mWriteEncoder = std::make_unique<gold::FrameEncoder>(mRenderResources);
		mReadEncoder = std::make_unique<gold::FrameEncoder>(mRenderResources);

		mFrameAllocator = std::make_unique<gold::LinearAllocator>(malloc(1024 * 1024 * 128), 1024 * 1024 * 128);
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
		using namespace gold; 
		using namespace graphics;


		BinaryReader reader = mReadEncoder->GetReader();

		bool complete = false;
		while (reader.HasData() && !complete)
		{
			RenderCommand command = reader.Read<RenderCommand>();
			switch (command)
			{

			// Uniform Buffers
			case RenderCommand::CreateUniformBuffer:
			{
				UniformBufferHandle clientHandle = reader.Read<UniformBufferHandle>();
				UniformBufferHandle& serverHandle = mRenderResources.get(clientHandle);

				u64 size = reader.Read<u64>();
				u8* data = (u8*)mFrameAllocator->Allocate(size);
				reader.Read(data, size);

				serverHandle = renderer.CreateUniformBlock(data, size);
				break;
			}
			case RenderCommand::UpdateUniformBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DestroyUniformBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			
			// Shader Buffers
			case RenderCommand::CreateShaderBuffer:
			{
				ShaderBufferHandle clientHandle = reader.Read<ShaderBufferHandle>();
				ShaderBufferHandle& serverHandle = mRenderResources.get(clientHandle);

				u64 size = reader.Read<u64>();
				u8* data = (u8*)mFrameAllocator->Allocate(size);
				reader.Read(data, size);

				serverHandle = renderer.CreateStorageBlock(data, size);
				break;
			}
			case RenderCommand::UpdateShaderBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DestroyShaderBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}

			// Vertex Buffers
			case RenderCommand::CreateVertexBuffer:
			{
				VertexBufferHandle clientHandle = reader.Read<VertexBufferHandle>();
				VertexBufferHandle& serverHandle = mRenderResources.get(clientHandle);
				
				u64 size = reader.Read<u64>();
				u8* data = (u8*)mFrameAllocator->Allocate(size);
				reader.Read(data, size);

				serverHandle = renderer.CreateVertexBuffer(data, size);
				break;
			}
			case RenderCommand::UpdateVertexBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DestroyVertexBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}

			// Index Buffers
			case RenderCommand::CreateIndexBuffer:
			{
				IndexBufferHandle clientHandle = reader.Read<IndexBufferHandle>();
				IndexBufferHandle& serverHandle = mRenderResources.get(clientHandle);
				
				u64 size = reader.Read<u64>();
				u8* data = (u8*)mFrameAllocator->Allocate(size);
				reader.Read(data, size);

				serverHandle = renderer.CreateIndexBuffer(data, size);
				break;
			}
			case RenderCommand::UpdateIndexBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DestroyIndexBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}

			// Shaders
			case RenderCommand::CreateShader:
			{
				ShaderHandle clientHandle = reader.Read<ShaderHandle>();
				ShaderHandle& serverHandle = mRenderResources.get(clientHandle);

				u64 vertSize = reader.Read<u64>();
				char* vertSrc = (char*)mFrameAllocator->Allocate(vertSize);
				reader.Read((u8*)vertSrc, vertSize);

				u64 fragSize = reader.Read<u64>();
				char* fragSrc = (char*)mFrameAllocator->Allocate(fragSize);
				reader.Read((u8*)fragSrc, fragSize);

				serverHandle = renderer.CreateShader(vertSrc, fragSrc);
				break;
			}
			case RenderCommand::DestroyShader:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}

			// Textures
			case RenderCommand::CreateTexture2D:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::CreateTexture3D:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::CreateCubemap:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DestroyTexture:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}

			// Frame Buffers
			case RenderCommand::CreateFrameBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DestroyFrameBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}

			// Draw Calls
			case RenderCommand::DrawMesh:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DrawMeshInstanced:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DispatchCompute:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}

			// Render Pass
			case RenderCommand::AddRenderPass:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}

			case RenderCommand::END:
			{
				complete = true;
				break;
			}
		}
		

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
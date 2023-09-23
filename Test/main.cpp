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

	
	
public:
	TestApp(gold::ApplicationConfig&& config)
		: gold::Application(std::move(config))
	{
	}

protected:

	void InitRenderData(gold::FrameEncoder& encoder)
	{
		glm::mat4 mvp;

		glm::vec2 screenSize = GetScreenSize();

		mvp = glm::perspective(glm::radians(65.f), screenSize.x / screenSize.y, 1.f, 100.f);
		mvp *= glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0,0,0 }, glm::vec3{ 0,1,0 });

		mView = encoder.CreateUniformBuffer(&mvp, sizeof(glm::mat4));

		std::string vertSrc = util::LoadStringFromFile("shaders/default.vert.glsl");

		std::string fragSrc = util::LoadStringFromFile("shaders/default.frag.glsl");
		mShader = encoder.CreateShader(vertSrc.c_str(), fragSrc.c_str());


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
		mesh.mIndices = encoder.CreateIndexBuffer(indices.data(), sizeof(uint16_t) * indices.size());
		mesh.mIndexCount = 3;

		mesh.mVertexCount = 3;
		mesh.mStride = buffer.GetLayout().Size();
		mesh.offsets.mPositionOffset = buffer.GetLayout().Resolve<graphics::VertexLayout::Position3>().GetOffset();
		mesh.offsets.mColorsOffset = buffer.GetLayout().Resolve<graphics::VertexLayout::Color3>().GetOffset();

		mesh.mInterlacedBuffer = encoder.CreateVertexBuffer(buffer.Raw(), buffer.SizeInBytes());

		mMesh = encoder.CreateMesh(mesh);
	}

	virtual void Init() override
	{
	}


	virtual void Update(float delta, gold::FrameEncoder& encoder) override
	{
		encoder.Begin();

		if (mFirstFrame)
		{
			InitRenderData(encoder);
			mFirstFrame = false;
		}

		// TODO (danielg): updates not supported yet
		//glm::vec2 screenSize = GetScreenSize();

		//glm::mat4 mvp = glm::perspective(glm::radians(65.f), screenSize.x / screenSize.y, 1.f, 100.f);
		//mvp *= glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0,0,0 }, glm::vec3{ 0,1,0 });
		//renderer.UpdateUniformBlock(mvp, mView);

		/*renderer.SetBackBufferSize((int)screenSize.x, (int)screenSize.y);
		renderer.BeginFrame();*/

		uint8_t pass = encoder.AddRenderPass("Default", graphics::ClearColor::YES, graphics::ClearDepth::YES);
		graphics::RenderState state;
		state.mRenderPass = pass;
		state.mShader = mShader;
		state.SetUniformBlock("View_UBO", { mView });

		encoder.DrawMesh(mMesh, state);

		encoder.End();
	}

	virtual void Render(gold::ServerResources& resources, graphics::Renderer& renderer, gold::BinaryReader& reader, gold::LinearAllocator& frameAllocator) override
	{
		using namespace gold; 
		using namespace graphics;

		auto remap = [&resources](auto clientHandle)
		{
			return clientHandle.idx == 0 ? clientHandle : resources.get(clientHandle);
		};

		renderer.BeginFrame();

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
				UniformBufferHandle& serverHandle = resources.get(clientHandle);

				u32 size = reader.Read<u32>();
				u8* data = (u8*)frameAllocator.Allocate(size);
				reader.Read(data, size);

				serverHandle = renderer.CreateUniformBlock(data, size);
				break;
			}
			/*case RenderCommand::UpdateUniformBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DestroyUniformBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}*/

			// Shader Buffers
			case RenderCommand::CreateShaderBuffer:
			{
				ShaderBufferHandle clientHandle = reader.Read<ShaderBufferHandle>();
				ShaderBufferHandle& serverHandle = resources.get(clientHandle);

				u32 size = reader.Read<u32>();
				u8* data = (u8*)frameAllocator.Allocate(size);
				reader.Read(data, size);

				serverHandle = renderer.CreateStorageBlock(data, size);
				break;
			}
			/*case RenderCommand::UpdateShaderBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DestroyShaderBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}*/

			// Vertex Buffers
			case RenderCommand::CreateVertexBuffer:
			{
				VertexBufferHandle clientHandle = reader.Read<VertexBufferHandle>();
				VertexBufferHandle& serverHandle = resources.get(clientHandle);

				u32 size = reader.Read<u32>();
				u8* data = (u8*)frameAllocator.Allocate(size);
				reader.Read(data, size);

				serverHandle = renderer.CreateVertexBuffer(data, size);
				break;
			}
			/*case RenderCommand::UpdateVertexBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DestroyVertexBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}*/

			// Index Buffers
			case RenderCommand::CreateIndexBuffer:
			{
				IndexBufferHandle clientHandle = reader.Read<IndexBufferHandle>();
				IndexBufferHandle& serverHandle = resources.get(clientHandle);

				u32 size = reader.Read<u32>();
				u8* data = (u8*)frameAllocator.Allocate(size);
				reader.Read(data, size);

				serverHandle = renderer.CreateIndexBuffer(data, size);
				break;
			}
			/*case RenderCommand::UpdateIndexBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DestroyIndexBuffer:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}*/

			// Shaders
			case RenderCommand::CreateShader:
			{
				ShaderHandle clientHandle = reader.Read<ShaderHandle>();

				u32 vertSize = reader.Read<u32>();
				char* vertSrc = (char*)frameAllocator.Allocate(vertSize);
				reader.Read((u8*)vertSrc, vertSize);

				u32 fragSize = reader.Read<u32>();
				char* fragSrc = (char*)frameAllocator.Allocate(fragSize);
				reader.Read((u8*)fragSrc, fragSize);

				resources.get(clientHandle) = renderer.CreateShader(vertSrc, fragSrc);
				break;
			}
			//case RenderCommand::DestroyShader:
			//{
			//	DEBUG_ASSERT(false, "Not implemented!");
			//	break;
			//}

			//// Textures
			//case RenderCommand::CreateTexture2D:
			//{
			//	DEBUG_ASSERT(false, "Not implemented!");
			//	break;
			//}
			//case RenderCommand::CreateTexture3D:
			//{
			//	DEBUG_ASSERT(false, "Not implemented!");
			//	break;
			//}
			//case RenderCommand::CreateCubemap:
			//{
			//	DEBUG_ASSERT(false, "Not implemented!");
			//	break;
			//}
			//case RenderCommand::DestroyTexture:
			//{
			//	DEBUG_ASSERT(false, "Not implemented!");
			//	break;
			//}

			//// Frame Buffers
			//case RenderCommand::CreateFrameBuffer:
			//{
			//	DEBUG_ASSERT(false, "Not implemented!");
			//	break;
			//}
			//case RenderCommand::DestroyFrameBuffer:
			//{
			//	DEBUG_ASSERT(false, "Not implemented!");
			//	break;
			//}

			case RenderCommand::CreateMesh:
			{
				MeshHandle clientHandle = reader.Read<MeshHandle>();
				MeshHandle& serverHandle = resources.get(clientHandle);

				MeshDescription desc;
				desc.mInterlacedBuffer = remap(reader.Read<VertexBufferHandle>());
				desc.mStride = reader.Read<u32>();

				if (desc.mInterlacedBuffer.idx)
				{
					desc.offsets.mPositionOffset	= reader.Read<u32>();
					desc.offsets.mNormalsOffset		= reader.Read<u32>();
					desc.offsets.mTexCoord0Offset	= reader.Read<u32>();
					desc.offsets.mTexCoord1Offset	= reader.Read<u32>();
					desc.offsets.mColorsOffset		= reader.Read<u32>();
					desc.offsets.mJointsOffset		= reader.Read<u32>();
					desc.offsets.mWeightsOffset		= reader.Read<u32>();
				}
				else
				{
					desc.handles.mPositions		= remap(reader.Read<VertexBufferHandle>());
					desc.handles.mNormals		= remap(reader.Read<VertexBufferHandle>());
					desc.handles.mTexCoords0	= remap(reader.Read<VertexBufferHandle>());
					desc.handles.mTexCoords1	= remap(reader.Read<VertexBufferHandle>());
					desc.handles.mColors		= remap(reader.Read<VertexBufferHandle>());
					desc.handles.mJoints		= remap(reader.Read<VertexBufferHandle>());
					desc.handles.mWeights		= remap(reader.Read<VertexBufferHandle>());
				}

				desc.mIndices = remap(reader.Read<IndexBufferHandle>());
				desc.mVertexCount = reader.Read<u32>();
				desc.mIndexCount = reader.Read<u32>();
				desc.mIndexStart = reader.Read<u32>();

				desc.mPrimitiveType = reader.Read <PrimitiveType>();

				desc.mPositionFormat = reader.Read<VertexFormat>();
				desc.mNormalsFormat = reader.Read<VertexFormat>();
				desc.mTexCoord0Format = reader.Read<VertexFormat>();
				desc.mTexCoord1Format = reader.Read<VertexFormat>();
				desc.mColorsFormat = reader.Read<VertexFormat>();
				desc.mJointsFormat = reader.Read<VertexFormat>();
				desc.mWeightsFormat = reader.Read<VertexFormat>();

				desc.mIndicesFormat = reader.Read<IndexFormat>();

				serverHandle = renderer.CreateMesh(desc);

				break;
			}

			// Draw Calls
			case RenderCommand::DrawMesh:
			{
				MeshHandle clientHandle = reader.Read<MeshHandle>();
				MeshHandle serverHandle = resources.get(clientHandle);

				RenderState state;

				// uniform buffers
				state.mNumUniformBlocks = reader.Read<u32>();
				for (u32 i = 0; i < state.mNumUniformBlocks; ++i)
				{
					RenderState::UniformBlock& buffer = state.mUniformBlocks[i];
					buffer.mNameHash = reader.Read<u32>();
					buffer.mBinding = remap(reader.Read<UniformBufferHandle>());
				}

				// shader buffers
				state.mNumStorageBlocks = reader.Read<u32>();
				for (u32 i = 0; i < state.mNumStorageBlocks; ++i)
				{
					RenderState::StorageBlock& buffer = state.mStorageBlocks[i];
					buffer.mNameHash = reader.Read<u32>();
					buffer.mBinding = remap(reader.Read<ShaderBufferHandle>());
				}

				// Textures
				state.mNumTextures = reader.Read<u32>();
				for (u32 i = 0; i < state.mNumTextures; ++i)
				{
					RenderState::Texture& texture = state.mTextures[i];
					texture.mNameHash = reader.Read<u32>();
					texture.mHandle = remap(reader.Read<TextureHandle>());
				}

				// Images
				state.mNumImages = reader.Read<u32>();
				for (u32 i = 0; i < state.mNumImages; ++i)
				{
					RenderState::Image& image = state.mImages[i];
					image.mNameHash = reader.Read<u32>();
					image.mHandle = remap(reader.Read<TextureHandle>());

					// TODO (danielg): Also defined in the frame encoder. Move to shared location
					u8 readBit = image.read   ? 1 << 0 : 0;
					u8 writeBit = image.write ? 1 << 1 : 0;
					
					u8 readWrite = reader.Read<u8>();

					image.read  = (readWrite & readBit)  > 0;
					image.write = (readWrite & writeBit) > 0;
				}

				// Render pass 
				state.mRenderPass = reader.Read<u8>();

				// Shader
				state.mShader = reader.Read<ShaderHandle>();
				state.mShader = resources.get(state.mShader);

				// viewport
				state.mViewport.x = reader.Read<int>();
				state.mViewport.y = reader.Read<int>();
				state.mViewport.width = reader.Read<int>();
				state.mViewport.height = reader.Read<int>();

				// Depth Func
				state.mDepthFunc = reader.Read<DepthFunction>();

				// Blend Func
				state.mSrcBlendFunc = reader.Read<BlendFunction>();
				state.mDstBlendFunc = reader.Read<BlendFunction>();

				// CullFace
				state.mCullFace = reader.Read<CullFace>();

				// TODO (danielg): Also defined in the frame encoder. Move to shared location
				// enable/disable booleans
				u8 depthWriteBit = 1 << 0;
				u8 colorWriteBit = 1 << 1;
				u8 alphaBlendBit = 1 << 2;
				u8 wireframeBit = 1 << 3;

				u8 toggles = reader.Read<u8>();

				state.mDepthWriteEnabled = (toggles & depthWriteBit) > 0;
				state.mColorWriteEnabled = (toggles & colorWriteBit) > 0;
				state.mAlphaBlendEnabled = (toggles & alphaBlendBit) > 0;
				state.mWireFrame		 = (toggles & wireframeBit)  > 0;

				renderer.DrawMesh(serverHandle, state);

				break;
			}
			/*case RenderCommand::DrawMeshInstanced:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}
			case RenderCommand::DispatchCompute:
			{
				DEBUG_ASSERT(false, "Not implemented!");
				break;
			}*/

			// Render Pass
			case RenderCommand::AddRenderPass:
			{
				RenderPass pass;

				u32 nameSize = reader.Read<u32>();
				pass.mName = (char*)frameAllocator.Allocate(nameSize);
				reader.Read((u8*)pass.mName, nameSize);
				

				pass.mTarget.mHandle = reader.Read<u32>();
				for (u32 i = 0; i < static_cast<u32>(OutputSlot::Count); ++i)
				{
					TextureHandle clientHandle = reader.Read<TextureHandle>();
					TextureHandle serverHandle = remap(clientHandle);
					pass.mTarget.mTextures[i] = serverHandle;
				}
				
				pass.mTarget.mWidth = reader.Read<u32>();
				pass.mTarget.mHeight = reader.Read<u32>();

				// TODO (danielg): Also defined in the frame encoder. Move to shared location
				// clear color/depth
				u8 clearColorBit = 1 << 0;
				u8 clearDepthBit = 1 << 1;
				u8 clearBits = reader.Read<u8>();

				pass.mClearColor = (clearColorBit & clearBits) == clearColorBit;
				pass.mClearDepth = (clearDepthBit & clearBits) == clearDepthBit;

				pass.mColor = reader.Read<glm::vec4>();
				pass.mDepth = reader.Read<float>();

				renderer.AddRenderPass(pass);

				break;
			}

			case RenderCommand::END:
			{
				complete = true;
				renderer.EndFrame();
				break;
			}
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
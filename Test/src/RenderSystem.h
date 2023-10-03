#pragma once

#include "scene/GameSystem.h"
#include "graphics/FrameEncoder.h"
#include "graphics/Texture.h"

class RenderSystem : scene::GameSystem
{
private:
	gold::FrameEncoder* mEncoder = nullptr;

	graphics::UniformBufferHandle mView{};
	graphics::ShaderHandle mShader{};

	graphics::TextureHandle mTexture{};

	graphics::MeshHandle mMesh{};
	
	graphics::FrameBuffer mTarget{};

	bool mFirstFrame = true;

	void InitRenderData()
	{
		glm::mat4 mvp;

		mvp = glm::perspective(glm::radians(65.f), (float)mTarget.mWidth / (float)mTarget.mHeight, 1.f, 100.f);
		mvp *= glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0,0,0 }, glm::vec3{ 0,1,0 });

		mView = mEncoder->CreateUniformBuffer(&mvp, sizeof(glm::mat4));

		std::string vertSrc = util::LoadStringFromFile("shaders/default.vert.glsl");

		std::string fragSrc = util::LoadStringFromFile("shaders/default.frag.glsl");
		mShader = mEncoder->CreateShader(vertSrc.c_str(), fragSrc.c_str());


		graphics::VertexLayout layout;
		layout.Push<graphics::VertexLayout::Position3>();
		layout.Push<graphics::VertexLayout::Texcoord2>();
		layout.Push<graphics::VertexLayout::Color3>();

		graphics::VertexBuffer buffer(std::move(layout));

		glm::vec3 pos0 = { -1, -1, 0 };
		glm::vec2 tex0 = { 0, 0 };
		glm::vec3 col0 = { 1, 0, 0 };

		glm::vec3 pos1 = { 1, -1, 0 };
		glm::vec2 tex1 = { 1, 0 };
		glm::vec3 col1 = { 0, 1, 0 };

		glm::vec3 pos2 = { 0, 1, 0 };
		glm::vec2 tex2 = { 0.5, 1.0};
		glm::vec3 col2 = { 0, 0, 1 };

		buffer.Emplace(pos0, tex0, col0);
		buffer.Emplace(pos1, tex1, col1);
		buffer.Emplace(pos2, tex2, col2);

		std::vector<uint16_t> indices =
		{
			0, 1, 2
		};

		graphics::MeshDescription mesh;
		mesh.mIndicesFormat = graphics::IndexFormat::U16;
		mesh.mIndices = mEncoder->CreateIndexBuffer(indices.data(), sizeof(uint16_t) * indices.size());
		mesh.mIndexCount = 3;

		mesh.mVertexCount = 3;
		mesh.mStride = buffer.GetLayout().Size();
		mesh.offsets.mPositionOffset = buffer.GetLayout().Resolve<graphics::VertexLayout::Position3>().GetOffset();
		mesh.offsets.mTexCoord0Offset = buffer.GetLayout().Resolve<graphics::VertexLayout::Texcoord2>().GetOffset();
		mesh.offsets.mColorsOffset = buffer.GetLayout().Resolve<graphics::VertexLayout::Color3>().GetOffset();

		mesh.mInterlacedBuffer = mEncoder->CreateVertexBuffer(buffer.Raw(), buffer.SizeInBytes());

		mMesh = mEncoder->CreateMesh(mesh);

		{
			graphics::Texture2D texture("textures/lava.png");
			graphics::TextureDescription2D desc(texture, false);
			desc.mFilter = graphics::TextureFilter::POINT;
			mTexture = mEncoder->CreateTexture2D(desc);
		}
	}

public:
	
	RenderSystem()
	{

	}
	
	virtual ~RenderSystem()
	{

	}

	void SetEncoder(gold::FrameEncoder* frameEncoder)
	{
		DEBUG_ASSERT(frameEncoder, "Frame Encoder should never be null!");
		mEncoder = frameEncoder;
	}

	void SetRenderTarget(graphics::FrameBuffer target)
	{
		mTarget = target;
	}

	virtual void Tick(scene::Scene& scene, float dt) override
	{
		if (mFirstFrame)
		{
			InitRenderData();
			mFirstFrame = false;
		}

		glm::mat4 mvp = glm::perspective(glm::radians(65.f), (float)mTarget.mWidth / (float)mTarget.mHeight, 1.f, 100.f);

		mvp *= glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0,0,0 }, glm::vec3{ 0,1,0 });
		mEncoder->UpdateUniformBuffer(mView, &mvp, sizeof(glm::mat4));

		uint8_t pass = mEncoder->AddRenderPass("Default", mTarget.mHandle, graphics::ClearColor::YES, graphics::ClearDepth::YES);
		graphics::RenderState state;
		state.mRenderPass = pass;
		state.mShader = mShader;
		state.SetUniformBlock("View_UBO", { mView });
		state.SetTexture("u_texture", mTexture);

		mEncoder->DrawMesh(mMesh, state);
	}
};
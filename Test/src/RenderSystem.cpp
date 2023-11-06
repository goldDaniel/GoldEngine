#include "RenderSystem.h"

#include <graphics/Vertex.h>

using namespace graphics;

void RenderSystem::InitRenderData(scene::Scene& scene)
{

	// Per frame constants
	{
		mPerFrameConstants.u_proj = glm::perspective(glm::radians(65.f), (float)mGBuffer.mWidth / (float)mGBuffer.mHeight, 1.f, 1000.f);
		mPerFrameConstants.u_projInv = glm::inverse(mPerFrameConstants.u_proj);

		mPerFrameConstants.u_view = glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0,0,0 }, glm::vec3{ 0,1,0 });
		mPerFrameConstants.u_viewInv = glm::inverse(mPerFrameConstants.u_view);
		mPerFrameConstants.u_time = { 0,0,0,0 };
		mPerFrameContantsBuffer = mEncoder->CreateUniformBuffer(&mPerFrameConstants, sizeof(PerFrameConstants));
	}

	// Per Draw constants
	{
		PerDrawConstants perDrawConstants{};
		mPerDrawConstantsBuffer = mEncoder->CreateUniformBuffer(&perDrawConstants, sizeof(PerDrawConstants));
	}

	// Lighting data constants
	{
		LightBufferComponent::LightShaderBuffer lightBuffer{};
		mLightingBuffer = mEncoder->CreateUniformBuffer(&lightBuffer, sizeof(LightBufferComponent::LightShaderBuffer));
	}

	//GBuffer fill
	{
		std::string vertSrc = util::LoadStringFromFile("shaders/gbuffer_fill.vert.glsl");
		std::string fragSrc = util::LoadStringFromFile("shaders/gbuffer_fill.frag.glsl");
		mGBufferFillShader = mEncoder->CreateShader(vertSrc.c_str(), fragSrc.c_str());
	}

	//GBuffer resolve
	{
		std::string vertSrc = util::LoadStringFromFile("shaders/gbuffer_resolve.vert.glsl");
		std::string fragSrc = util::LoadStringFromFile("shaders/gbuffer_resolve.frag.glsl");
		mGBufferResolveShader = mEncoder->CreateShader(vertSrc.c_str(), fragSrc.c_str());
	}

	//Tonemap
	{
		std::string vertSrc = util::LoadStringFromFile("shaders/tonemap.vert.glsl");
		std::string fragSrc = util::LoadStringFromFile("shaders/tonemap.frag.glsl");
		mTonemapShader = mEncoder->CreateShader(vertSrc.c_str(), fragSrc.c_str());
	}

	// fullscreen quad
	{
		MeshDescription desc;
		VertexBuffer buffer(std::move(VertexLayout().Push<VertexLayout::Position3>()
														  .Push<VertexLayout::Texcoord2>()));

		buffer.Emplace(glm::vec3{ -1.0f, -1.0f, 0.0f }, glm::vec2{ 0.0f, 0.0f });
		buffer.Emplace(glm::vec3{ 1.0f, -1.0f, 0.0f }, glm::vec2{ 1.0f, 0.0f });
		buffer.Emplace(glm::vec3{ 1.0f,  1.0f, 0.0f }, glm::vec2{ 1.0f, 1.0f });
		buffer.Emplace(glm::vec3{ -1.0f,  1.0f, 0.0f }, glm::vec2{ 0.0f, 1.0f });

		desc.mInterlacedBuffer = mEncoder->CreateVertexBuffer(buffer.Raw(), buffer.SizeInBytes());
		desc.mStride = buffer.GetLayout().Size();
		desc.offsets.mPositionOffset = 0;
		desc.offsets.mTexCoord0Offset = buffer.GetLayout().Resolve<VertexLayout::Texcoord2>().GetOffset();
		desc.mVertexCount = 4;

		std::vector<u16> indices{ 0, 1, 2, 3, 0, 2 };
		desc.mIndicesFormat = IndexFormat::U16;
		desc.mIndices = mEncoder->CreateIndexBuffer(indices.data(), sizeof(u16) * indices.size());
		desc.mIndexCount = 6;

		mFullscreenQuad = mEncoder->CreateMesh(desc);
	}
}


void RenderSystem::FillGBuffer(const Camera& camera, scene::Scene& scene)
{
	// frustum cull
	scene.ForEach<RenderComponent>([&camera](scene::GameObject obj)
	{
		auto aabb = obj.GetAABB();

		// invalid AABB? Add to draw list just in case
		if (aabb.min.x > aabb.max.x || aabb.min.y > aabb.max.y || aabb.min.z > aabb.max.z)
		{
			DEBUG_ASSERT(false, "Invalid AABB");
			obj.AddComponent<NotFrustumCulledComponent>();
			return;
		}

		if (!FrustumCuller::FrustumCulled(camera, aabb))
		{
			obj.AddComponent<NotFrustumCulledComponent>();
		}
	});

	//GBuffer fill
	uint8_t pass = mEncoder->AddRenderPass("GBuffer Fill", mGBuffer.mHandle, ClearColor::YES, ClearDepth::YES);
	scene.ForEach<TransformComponent, RenderComponent, NotFrustumCulledComponent>([&](const scene::GameObject obj)
	{
		const auto& render = obj.GetComponent<RenderComponent>();

		PerDrawConstants drawConstants =
		{
			obj.GetWorldSpaceTransform() ,
			render.albedo,
			render.emissive,
			{render.metallic, render.roughness, 0.f, render.uvScale},
			{render.albedoMap.idx, render.normalMap.idx,render.metallicMap.idx,render.roughnessMap.idx}
		};
		mEncoder->UpdateUniformBuffer(mPerDrawConstantsBuffer, &drawConstants, sizeof(PerDrawConstants));

		RenderState state{};
		state.mRenderPass = pass;
		state.mAlphaBlendEnabled = false;
		state.mShader = mGBufferFillShader;
		state.SetUniformBlock("PerFrameConstants_UBO", mPerFrameContantsBuffer);
		state.SetUniformBlock("PerDrawConstants_UBO", mPerDrawConstantsBuffer);

		if (render.albedoMap.idx)		state.SetTexture("u_albedoMap", render.albedoMap);
		if (render.normalMap.idx)		state.SetTexture("u_normalMap", render.normalMap);
		if (render.metallicMap.idx)		state.SetTexture("u_metallicMap", render.metallicMap);
		if (render.roughnessMap.idx)	state.SetTexture("u_roughnessMap", render.roughnessMap);

		mEncoder->DrawMesh(render.mesh, state);
	});

	// remove cull component for other passes
	//NOTE (danielg): instead, NotFrustumCulledComponent should use a bitset to
	//				  tell which camera the object has been culled from
	scene.ForEach<NotFrustumCulledComponent>([](scene::GameObject obj)
	{
		obj.RemoveComponent<NotFrustumCulledComponent>(); 
	});
}

void RenderSystem::Tick(scene::Scene& scene, float dt)
{
	if (mFirstFrame)
	{
		InitRenderData(scene);
		mFirstFrame = false;
	}

	// retrieve camera
	Camera* camera = nullptr;
	scene.ForEach<TransformComponent, DebugCameraComponent>([&, retrievedCamera = false](scene::GameObject obj) mutable
	{
		DEBUG_ASSERT(!retrievedCamera, "Currently only support one debug camera!");
		retrievedCamera = true;

		auto& cam = obj.GetComponent<DebugCameraComponent>();
		cam.mCamera.Aspect = (float)mGBuffer.mWidth / (float)mGBuffer.mHeight;
		camera = &cam.mCamera;
	});

	// update per frame buffer
	{
		mPerFrameConstants.u_proj = camera->GetProjectionMatrix();
		mPerFrameConstants.u_projInv = glm::inverse(mPerFrameConstants.u_proj);

		mPerFrameConstants.u_view = camera->GetViewMatrix();
		mPerFrameConstants.u_viewInv = glm::inverse(mPerFrameConstants.u_view);
		mPerFrameConstants.u_time.x += dt;

		mEncoder->UpdateUniformBuffer(mPerFrameContantsBuffer, &mPerFrameConstants, sizeof(PerFrameConstants));
	}

	FillGBuffer(*camera, scene);
	ResolveGBuffer(scene);
	DrawSkybox();
	Tonemap();
}

void RenderSystem::ResolveGBuffer(scene::Scene& scene)
{
	// update lighting buffers 
	// NOTE (danielg): mutable lambda to assert if there is more than 1 iteration
	// we only want one of these components, holds ALL lighting info
	scene.ForEach<LightBufferComponent>([this, onlyOneBuffer = true](scene::GameObject obj) mutable
	{
		DEBUG_ASSERT(onlyOneBuffer, "More than 1 LightBufferComponent found!");

		auto& buffer = obj.GetComponent<LightBufferComponent>();
		if (buffer.isDirty)
		{
			mEncoder->UpdateUniformBuffer(mLightingBuffer, &buffer.lightBuffer, sizeof(LightBufferComponent::LightShaderBuffer));
			buffer.isDirty = false;
		}
		onlyOneBuffer = false;
	});

	RenderPass pass;
	pass.mName = "GBuffer Resolve";
	pass.mTarget = mHDRBuffer.mHandle;
	pass.mClearColor = true;
	pass.mClearDepth = true;

	RenderState state;
	state.mRenderPass = mEncoder->AddRenderPass(pass);
	state.mShader = mGBufferResolveShader;
	state.mAlphaBlendEnabled = false;

	state.SetUniformBlock("PerFrameConstants_UBO", mPerFrameContantsBuffer);
	state.SetUniformBlock("Lights_UBO", mLightingBuffer);
	/*state.SetUniformBlock("LightSpaceMatrices", mLightMatricesBuffer);
	state.SetUniformBlock("ShadowPages", mShadowPagesBuffer);*/

	state.SetTexture("albedos", mGBuffer.mTextures[static_cast<uint8_t>(OutputSlot::Color0)]);
	state.SetTexture("normals", mGBuffer.mTextures[static_cast<uint8_t>(OutputSlot::Color1)]);
	state.SetTexture("coefficients", mGBuffer.mTextures[static_cast<uint8_t>(OutputSlot::Color2)]);
	state.SetTexture("depth", mGBuffer.mTextures[static_cast<uint8_t>(OutputSlot::Depth)]);

	//state.SetTexture("shadowMap", ShadowMapService::Get().GetTexture());

	mEncoder->DrawMesh(mFullscreenQuad, state);
}

void RenderSystem::DrawSkybox()
{
	/*RenderPass pass;
	pass.mName = "skybox_pass";
	pass.mTarget = mHDRBuffer.mHandle;

	RenderState state;
	state.mDepthFunc = DepthFunction::LESS_EQUAL;
	state.mCullFace = CullFace::FRONT;
	state.mRenderPass = mEncoder->AddRenderPass(pass);
	state.mShader = &mSkyboxShader;

	Renderer::UpdateUniformBlock(mConstants.mProj * glm::mat4(glm::mat3(mConstants.mView)), mSkyboxBuffer);
	state.SetUniformBlock("Skybox", mSkyboxBuffer);
	state.SetTexture("u_cubemap", mCubemap);

	Renderer::DrawMesh(mCube, state);*/
}

void RenderSystem::Tonemap()
{
	RenderState state;
	state.mDepthFunc = DepthFunction::ALWAYS;
	state.mAlphaBlendEnabled = false;
	state.mShader = mTonemapShader;
	state.mRenderPass = mEncoder->AddRenderPass("Tonemapping", mTonemapResultBuffer.mHandle, ClearColor::YES, ClearDepth::NO);
	state.SetTexture("hdrBuffer", mHDRBuffer.mTextures[static_cast<uint32_t>(OutputSlot::Color0)]);

	mEncoder->DrawMesh(mFullscreenQuad, state);
}

void RenderSystem::ResizeGBuffer(int width, int height)
{
	if (mGBuffer.mHandle.idx == 0 ||
		mGBuffer.mWidth != width || mGBuffer.mHeight != height)
	{
		// GBuffer
		if (IsValid(mGBuffer.mHandle))
		{
			mEncoder->DestroyFrameBuffer(mGBuffer.mHandle);
			mGBuffer.mHandle.idx = 0;
		}
		{
			FrameBufferDescription fbDesc;

			TextureDescription2D albedoDesc;
			albedoDesc.mWidth = width;
			albedoDesc.mHeight = height;
			albedoDesc.mFormat = TextureFormat::RGB_U8;
			fbDesc.Put(OutputSlot::Color0, albedoDesc);

			TextureDescription2D normalDesc;
			normalDesc.mWidth = width;
			normalDesc.mHeight = height;
			normalDesc.mFormat = TextureFormat::RGB_FLOAT;
			fbDesc.Put(OutputSlot::Color1, normalDesc);

			TextureDescription2D coeffDesc;
			coeffDesc.mWidth = width;
			coeffDesc.mHeight = height;
			coeffDesc.mFormat = TextureFormat::RGB_U8;
			fbDesc.Put(OutputSlot::Color2, coeffDesc);

			TextureDescription2D depthDesc;
			depthDesc.mWidth = width;
			depthDesc.mHeight = height;
			depthDesc.mFormat = TextureFormat::DEPTH;
			fbDesc.Put(OutputSlot::Depth, depthDesc);

			mGBuffer = mEncoder->CreateFrameBuffer(fbDesc);
		}

		// HDR Buffer
		if (IsValid(mHDRBuffer.mHandle))
		{
			mEncoder->DestroyFrameBuffer(mHDRBuffer.mHandle);
		}
		{
			FrameBufferDescription fbDesc;

			TextureDescription2D colorDesc;
			colorDesc.mWidth = width;
			colorDesc.mHeight = height;
			colorDesc.mFormat = TextureFormat::RGBA_FLOAT;
			colorDesc.mWrap = TextureWrap::CLAMP;
			fbDesc.Put(OutputSlot::Color0, colorDesc);

			TextureDescription2D depthDesc;
			depthDesc.mWidth = width;
			depthDesc.mHeight = height;
			depthDesc.mFormat = TextureFormat::DEPTH;
			fbDesc.Put(OutputSlot::Depth, depthDesc);

			mHDRBuffer = mEncoder->CreateFrameBuffer(fbDesc);
		}

		// Tinemap result Buffer
		if (IsValid(mTonemapResultBuffer.mHandle))
		{
			mEncoder->DestroyFrameBuffer(mTonemapResultBuffer.mHandle);
		}
		{
			FrameBufferDescription fbDesc{};

			TextureDescription2D colorDesc;
			colorDesc.mWidth = width;
			colorDesc.mHeight = height;
			colorDesc.mFormat = TextureFormat::RGBA_U8;
			colorDesc.mWrap = TextureWrap::CLAMP;
			fbDesc.Put(OutputSlot::Color0, colorDesc);

			mTonemapResultBuffer = mEncoder->CreateFrameBuffer(fbDesc);
		}
	}
}
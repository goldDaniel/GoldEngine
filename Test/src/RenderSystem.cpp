#include "RenderSystem.h"

void RenderSystem::InitRenderData(scene::Scene& scene)
{
	mPerFrameConstants.u_proj = glm::perspective(glm::radians(65.f), (float)mGBuffer.mWidth / (float)mGBuffer.mHeight, 1.f, 1000.f);
	mPerFrameConstants.u_projInv = glm::inverse(mPerFrameConstants.u_proj);

	mPerFrameConstants.u_view = glm::lookAt(glm::vec3{ 0, 0, 5 }, glm::vec3{ 0,0,0 }, glm::vec3{ 0,1,0 });
	mPerFrameConstants.u_viewInv = glm::inverse(mPerFrameConstants.u_view);
	mPerFrameConstants.u_time = { 0,0,0,0 };
	mPerFrameContantsBuffer = mEncoder->CreateUniformBuffer(&mPerFrameConstants, sizeof(PerFrameConstants));

	PerDrawConstants perDrawConstants{};
	mPerDrawConstantsBuffer = mEncoder->CreateUniformBuffer(&perDrawConstants, sizeof(PerDrawConstants));

	std::string vertSrc = util::LoadStringFromFile("shaders/gbuffer_fill.vert.glsl");
	std::string fragSrc = util::LoadStringFromFile("shaders/gbuffer_fill.frag.glsl");
	mShader = mEncoder->CreateShader(vertSrc.c_str(), fragSrc.c_str());
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
	uint8_t pass = mEncoder->AddRenderPass("Default", mGBuffer.mHandle, graphics::ClearColor::YES, graphics::ClearDepth::YES);
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

		graphics::RenderState state;
		state.mRenderPass = pass;
		state.mAlphaBlendEnabled = false;
		state.mShader = mShader;
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
	bool retrievedCamera = false;
	scene.ForEach<TransformComponent, DebugCameraComponent>([&](scene::GameObject obj)
	{
		DEBUG_ASSERT(!retrievedCamera, "Currently only support one debug camera!");
		auto& cam = obj.GetComponent<DebugCameraComponent>();
		cam.mCamera.Aspect = (float)mGBuffer.mWidth / (float)mGBuffer.mHeight;
		camera = &cam.mCamera;
		retrievedCamera = true;
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
}

void RenderSystem::ResizeGBuffer(int width, int height)
{
	if (mGBuffer.mHandle.idx == 0 ||
		mGBuffer.mWidth != width || mGBuffer.mHeight != height)
	{
		if (mGBuffer.mHandle.idx)
		{
			mEncoder->DestroyFrameBuffer(mGBuffer.mHandle);
			mGBuffer.mHandle.idx = 0;
		}

		{
			using namespace graphics;
			FrameBufferDescription fbDesc;

			TextureDescription2D albedoDesc;
			albedoDesc.mWidth = width;
			albedoDesc.mHeight = height;
			albedoDesc.mFormat = TextureFormat::RGB_U8;
			fbDesc.Put(graphics::OutputSlot::Color0, albedoDesc);

			TextureDescription2D normalDesc;
			normalDesc.mWidth = width;
			normalDesc.mHeight = height;
			normalDesc.mFormat = TextureFormat::RGB_FLOAT;
			fbDesc.Put(graphics::OutputSlot::Color1, normalDesc);

			TextureDescription2D coeffDesc;
			coeffDesc.mWidth = width;
			coeffDesc.mHeight = height;
			coeffDesc.mFormat = TextureFormat::RGB_U8;
			fbDesc.Put(graphics::OutputSlot::Color2, coeffDesc);

			TextureDescription2D depthDesc;
			depthDesc.mWidth = width;
			depthDesc.mHeight = height;
			depthDesc.mFormat = TextureFormat::DEPTH;
			fbDesc.Put(graphics::OutputSlot::Depth, depthDesc);

			mGBuffer = mEncoder->CreateFrameBuffer(fbDesc);
		}
	}
}
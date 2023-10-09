#pragma once

#include "scene/BaseComponents.h"
#include "scene/GameSystem.h"
#include "graphics/FrameEncoder.h"
#include "graphics/Texture.h"
#include "graphics/FrustumCuller.h"

#include "Components.h"

class RenderSystem : scene::GameSystem
{
private:
	gold::FrameEncoder* mEncoder = nullptr;


	struct PerFrameConstants
	{
		glm::mat4 u_proj{};
		glm::mat4 u_projInv{};

		glm::mat4 u_view{};
		glm::mat4 u_viewInv{};

		glm::vec4 u_time{};
	};
	PerFrameConstants mPerFrameConstants{};
	graphics::UniformBufferHandle mPerFrameContantsBuffer{};

	struct PerDrawConstants
	{
		glm::mat4 u_model{};
		
		// for use when maps are not present
		glm::vec4 u_albedo{};
		glm::vec4 u_emissive{};
		glm::vec4 u_coefficients{};// metallic, roughness, ?, uvScale

		glm::vec4 u_flags; // albedoMap, normalMap, metallicMap, roughnessMap
	};
	graphics::UniformBufferHandle mPerDrawConstantsBuffer{};

	graphics::ShaderHandle mShader{};

	graphics::FrameBuffer mTarget{};

	bool mFirstFrame = true;

	void InitRenderData(scene::Scene& scene)
	{
		mPerFrameConstants.u_proj = glm::perspective(glm::radians(65.f), (float)mTarget.mWidth / (float)mTarget.mHeight, 1.f, 1000.f);
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
			InitRenderData(scene);
			mFirstFrame = false;
		}
		
		
		


		glm::mat4 view{};
		float nearPlane{};
		float farPlane{};
		float aspect = (float)mTarget.mWidth / (float)mTarget.mHeight;
		scene.ForEach<TransformComponent, DebugCameraComponent>([&](scene::GameObject obj)
		{
			const auto& cam = obj.GetComponent<DebugCameraComponent>();
			view = cam.GetViewMatrix();
			nearPlane = cam.mCamera.Near;
			farPlane = cam.mCamera.Far;
		});
		
		// update per frame buffer
		{
			mPerFrameConstants.u_proj = glm::perspective(glm::radians(65.f), (float)mTarget.mWidth / (float)mTarget.mHeight, 1.f, 1000.f);
			mPerFrameConstants.u_projInv = glm::inverse(mPerFrameConstants.u_proj);

			mPerFrameConstants.u_view = view;
			mPerFrameConstants.u_viewInv = glm::inverse(mPerFrameConstants.u_view);
			mPerFrameConstants.u_time.x += dt;

			mEncoder->UpdateUniformBuffer(mPerFrameContantsBuffer, &mPerFrameConstants, sizeof(PerFrameConstants));
		}

		// frustum cull
		scene.ForEach<RenderComponent>([&](scene::GameObject obj)
		{
			auto aabb = obj.GetAABB();

			glm::vec3 min = std::get<0>(aabb);
			glm::vec3 max = std::get<1>(aabb);

			// invalid AABB? Add to draw list just in case
			if (min.x > max.x || min.y > max.y || min.z > max.z)
			{
				DEBUG_ASSERT(false, "Invalid AABB");	
				obj.AddComponent<NotFrustumCulledComponent>();
				return;
			}

			obj.AddComponent<NotFrustumCulledComponent>();
		});

		uint8_t pass = mEncoder->AddRenderPass("Default", mTarget.mHandle, graphics::ClearColor::YES, graphics::ClearDepth::YES);
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

		scene.ForEach<NotFrustumCulledComponent>([](scene::GameObject obj) { obj.RemoveComponent<NotFrustumCulledComponent>(); });
	}
};
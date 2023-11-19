#include "RenderSystem.h"

#include <core/Core.h>
#include <graphics/Vertex.h>
#include "ShadowMapService.h"

using namespace graphics;

static void PushFrustumCull(scene::Scene& scene, const glm::mat4& viewProj)
{
	// frustum cull
	const glm::mat4 transViewProj = glm::transpose(viewProj);
	const glm::mat4 invViewProj = glm::inverse(viewProj);
	scene.ForEach<RenderComponent>([&invViewProj, &transViewProj](scene::GameObject obj)
	{
		auto aabb = obj.GetAABB();

		// invalid AABB? Add to draw list just in case
		if (aabb.min.x > aabb.max.x || aabb.min.y > aabb.max.y || aabb.min.z > aabb.max.z)
		{
			DEBUG_ASSERT(false, "Invalid AABB");
			obj.AddComponent<NotFrustumCulledComponent>();
			return;
		}

		if (!FrustumCuller::FrustumCulled(invViewProj, transViewProj, aabb))
		{
			obj.AddComponent<NotFrustumCulledComponent>();
		}
	});
}

static void PopFrustumCull(scene::Scene& scene)
{
	// remove cull component for other passes
	//NOTE (danielg): instead, NotFrustumCulledComponent should use a bitset to
	//				  tell which camera the object has been culled from
	scene.ForEach<NotFrustumCulledComponent>([](scene::GameObject obj)
	{
		obj.RemoveComponent<NotFrustumCulledComponent>();
	});
}

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

	// shadow map atlas framebuffer
	{
		TextureDescription2D texDesc;
		texDesc.mWidth = ShadowMapService::kTextureWidth;
		texDesc.mHeight = ShadowMapService::kTextureHeight;

		// TODO (danielg): point sampling to avoid blending between pages. eventually figure out linear blending? border color?
		texDesc.mFilter = TextureFilter::POINT;
		texDesc.mFormat = TextureFormat::DEPTH;
		texDesc.mMipmaps = false;

		FrameBufferDescription fbDesc;
		fbDesc.Put<OutputSlot::Depth>(texDesc);
		mShadowMapFrameBuffer = mEncoder->CreateFrameBuffer(fbDesc);
	}

	// Light matrices
	{
		mLightMatricesBuffer = mEncoder->CreateUniformBuffer(&mLightMatrices, sizeof(LightMatrices));
	}

	// Shadow atlas pages
	{
		mShadowPagesBuffer = mEncoder->CreateUniformBuffer(&mShadowPages, sizeof(ShadowMapPages));
	}

	//Shadow Atlas Fill
	{
		std::string vertSrc = util::LoadStringFromFile("shaders/shadow.vert.glsl");
		std::string fragSrc = util::LoadStringFromFile("shaders/shadow.frag.glsl");
		mShadowAtlasFillShader = mEncoder->CreateShader(vertSrc.c_str(), fragSrc.c_str());
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

	// Skybox
	{
		std::string vertSrc = util::LoadStringFromFile("shaders/skybox.vert.glsl");
		std::string fragSrc = util::LoadStringFromFile("shaders/skybox.frag.glsl");
		mSkyboxShader = mEncoder->CreateShader(vertSrc.c_str(), fragSrc.c_str());
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

	// cube
	{
		VertexBuffer posBuffer(std::move(VertexLayout().Push<VertexLayout::Position3>()));
		VertexBuffer norBuffer(std::move(VertexLayout().Push<VertexLayout::Normal>()));
		VertexBuffer uvBuffer(std::move(VertexLayout().Push<VertexLayout::Texcoord2>()));

		std::vector<glm::vec3> frontFace =
		{
			glm::vec3{ -0.5f, -0.5f, 0.5f },
			glm::vec3{  0.5f, -0.5f, 0.5f },
			glm::vec3{  0.5f,  0.5f, 0.5f },
			glm::vec3{ -0.5f,  0.5f, 0.5f },
			glm::vec3{ -0.5f, -0.5f, 0.5f },
			glm::vec3{  0.5f,  0.5f, 0.5f }
		};
		std::vector<glm::vec3> frontNor =
		{
			glm::vec3{ 0.0f, 0.0f, 1.0f },
			glm::vec3{ 0.0f, 0.0f, 1.0f },
			glm::vec3{ 0.0f, 0.0f, 1.0f },
			glm::vec3{ 0.0f, 0.0f, 1.0f },
			glm::vec3{ 0.0f, 0.0f, 1.0f },
			glm::vec3{ 0.0f, 0.0f, 1.0f }
		};

		std::vector<glm::vec2> frontUV =
		{
			glm::vec2{ 0.0f, 0.0f },
			glm::vec2{ 1.0f, 0.0f },
			glm::vec2{ 1.0f, 1.0f },
			glm::vec2{ 0.0f, 1.0f },
			glm::vec2{ 0.0f, 0.0f },
			glm::vec2{ 1.0f, 1.0f }
		};

		std::vector<glm::mat4> rotations;
		rotations.push_back(glm::rotate(glm::mat4(1.0f), glm::radians(0.f), glm::vec3(0.f, 1.f, 0.f)));
		rotations.push_back(glm::rotate(glm::mat4(1.0f), glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f)));
		rotations.push_back(glm::rotate(glm::mat4(1.0f), glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f)));
		rotations.push_back(glm::rotate(glm::mat4(1.0f), glm::radians(270.f), glm::vec3(0.f, 1.f, 0.f)));
		rotations.push_back(glm::rotate(glm::mat4(1.0f), glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f)));
		rotations.push_back(glm::rotate(glm::mat4(1.0f), glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f)));

		for (const auto& rot : rotations)
		{
			for (const auto& vert : frontFace)
			{
				posBuffer.Emplace(glm::vec3(rot * glm::vec4(vert, 1.0)));
			}
			for (const auto& nor : frontNor)
			{
				norBuffer.Emplace(glm::vec3(rot * glm::vec4(nor, 0.0)));
			}
			for (const auto& uv : frontUV)
			{
				uvBuffer.Emplace(uv);
			}
		}

		MeshDescription desc;
		desc.handles.mPositions = mEncoder->CreateVertexBuffer(posBuffer.Raw(), posBuffer.SizeInBytes());
		desc.handles.mNormals = mEncoder->CreateVertexBuffer(norBuffer.Raw(), norBuffer.SizeInBytes());
		desc.handles.mTexCoords0 = mEncoder->CreateVertexBuffer(uvBuffer.Raw(), uvBuffer.SizeInBytes());
		desc.mVertexCount = posBuffer.VertexCount();

		mCube = mEncoder->CreateMesh(desc);
	}

	// cubemap texture
	{
		std::unordered_map<CubemapFace, Texture2D&> faces;
		auto right = Texture2D("textures/cubemap/posx.jpg");
		auto left = Texture2D("textures/cubemap/negx.jpg");
		auto top = Texture2D("textures/cubemap/posy.jpg");
		auto bottom = Texture2D("textures/cubemap/negy.jpg");
		auto back = Texture2D("textures/cubemap/posz.jpg");
		auto front = Texture2D("textures/cubemap/negz.jpg");
		faces.emplace(CubemapFace::POSITIVE_X, right);
		faces.emplace(CubemapFace::NEGATIVE_X, left);
		faces.emplace(CubemapFace::POSITIVE_Y, top);
		faces.emplace(CubemapFace::NEGATIVE_Y, bottom);
		faces.emplace(CubemapFace::POSITIVE_Z, front);
		faces.emplace(CubemapFace::NEGATIVE_Z, back);

		CubemapDescription desc(faces);
		mCubemap = mEncoder->CreateCubemap(desc);
	}
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

	FillShadowAtlas(scene);
	FillGBuffer(*camera, scene);
	ResolveGBuffer(scene);
	DrawSkybox();
	Tonemap();
}

void RenderSystem::FillShadowAtlas(scene::Scene& scene)
{
	RenderPass shadowPass;
	shadowPass.mName = "shadow_pass";
	shadowPass.mClearDepth = true;
	shadowPass.mTarget = mShadowMapFrameBuffer.mHandle;

	RenderState shadowState;
	shadowState.mCullFace = CullFace::DISABLED;
	shadowState.mColorWriteEnabled = false;
	shadowState.mRenderPass = mEncoder->AddRenderPass(shadowPass);
	shadowState.mShader = mShadowAtlasFillShader;
	shadowState.SetUniformBlock("ShadowMap_UBO", mPerDrawConstantsBuffer);

	// directional light shadows
	scene.ForEach<DirectionalLightComponent, ShadowMapComponent>(
	[&scene, &shadowState, this](scene::GameObject obj) 
	{
		auto& shadow = obj.GetComponent<ShadowMapComponent>(); // not const so we can modify the dirty flag
		const auto& light = obj.GetComponent<DirectionalLightComponent>();
		const auto& pages = Singletons::Get()->Resolve<ShadowMapService>()->GetPages();

		if (!shadow.dirty) return;
		shadow.dirty = false;

		int shadowIndex = -1;
		for (const auto& page : pages)
		{
			if (page.shadowMapIndex == shadow.shadowMapIndex[0])
			{
				shadowIndex = page.shadowMapIndex;
				break;
			}
		}
		DEBUG_ASSERT(shadowIndex != -1, "Shadow map page ID assignment logic broken");

		const auto& page = Singletons::Get()->Resolve<ShadowMapService>()->GetPage(shadowIndex);

		// data for shadow pages uniform buffer
		mShadowPages.mPage[shadowIndex] = { page.x, page.y, page.width, page.height };
		mShadowPages.mParams[shadowIndex].w = shadow.shadowMapBias[0];
		mShadowPages.mParams[shadowIndex].z = shadow.PCFSize + 1;

		// data for light matrices uniform buffer
		// NOTE (danielg): adds support for light to follow a position
		glm::mat4 lightView = glm::lookAt(-glm::vec3(light.direction), glm::vec3(0,0,0), glm::vec3(0.0f, 1.0f, 0.0f));

		mLightMatrices.mLightSpace[shadowIndex] = glm::ortho(shadow.left, shadow.right, shadow.bottom, shadow.top, shadow.nearPlane, shadow.farPlane) * lightView;
		mLightMatrices.mLightInv[shadowIndex] = glm::inverse(mLightMatrices.mLightSpace[shadowIndex]);

		//the section of our paged shadowMap to render to 
		shadowState.mViewport = { page.x, page.y, page.width, page.height };

		PushFrustumCull(scene, mLightMatrices.mLightSpace[shadowIndex]);

		scene.ForEach<TransformComponent, RenderComponent, NotFrustumCulledComponent>(
		[&](scene::GameObject obj)
		{
			const auto& render = obj.GetComponent<RenderComponent>();
		
			// reusing the perDrawConstantsBuffer for the model matrix slot
			PerDrawConstants draw;
			draw.u_model = mLightMatrices.mLightSpace[shadowIndex] * obj.GetWorldSpaceTransform();
			mEncoder->UpdateUniformBuffer(mPerDrawConstantsBuffer, &draw, sizeof(PerDrawConstants));

			mEncoder->DrawMesh(render.mesh, shadowState);
		});

		PopFrustumCull(scene);
	});

	// directional lights
	scene.ForEach<PointLightComponent, ShadowMapComponent>([this, &shadowState, &scene](scene::GameObject& obj)
	{
		const auto& transform = obj.GetComponent<TransformComponent>();
		const auto& light = obj.GetComponent<PointLightComponent>();
		const auto& shadow = obj.GetComponent<ShadowMapComponent>();

		const auto& pages = Singletons::Get()->Resolve<ShadowMapService>()->GetPages();

		const std::array<glm::mat4, 6> lightViews
		{
			glm::lookAt(transform.position, transform.position + glm::vec3(1,0,0), glm::vec3(0,-1,0)), // +X
			glm::lookAt(transform.position, transform.position - glm::vec3(1,0,0), glm::vec3(0,-1,0)), // -X

			glm::lookAt(transform.position, transform.position + glm::vec3(0,1,0), glm::vec3(0,0,1)), // +Y
			glm::lookAt(transform.position, transform.position - glm::vec3(0,1,0), glm::vec3(0,0,-1)), // -Y

			glm::lookAt(transform.position, transform.position + glm::vec3(0,0,1), glm::vec3(0,-1,0)), // +Z
			glm::lookAt(transform.position, transform.position - glm::vec3(0,0,1), glm::vec3(0,-1,0)), // -Z
		};

		// for every face in the virtual cube map
		for (uint32_t i = 0; i < shadow.shadowMapIndex.size(); ++i)
		{
			int shadowIndex = -1;
			for (const auto& page : pages)
			{
				if (page.shadowMapIndex == shadow.shadowMapIndex[i])
				{
					shadowIndex = page.shadowMapIndex;
					break;
				}
			}

			DEBUG_ASSERT(shadowIndex != -1, "Shadow map page ID assignment logic broken");

			const auto& page = Singletons::Get()->Resolve<ShadowMapService>()->GetPage(shadowIndex);

			// data for shadow pages uniform buffer
			if (shadowIndex < LightBufferComponent::MAX_CASTERS) // should never happen, but compiler warnings
			{
				mShadowPages.mPage[shadowIndex] = { page.x, page.y, page.width, page.height };
				mShadowPages.mParams[shadowIndex].w = shadow.shadowMapBias[i];
				mShadowPages.mParams[shadowIndex].z = shadow.PCFSize + 1;

				mLightMatrices.mLightSpace[shadowIndex] = glm::perspective(shadow.FOV, shadow.aspect, shadow.nearPlane, shadow.farPlane) * lightViews[i];
				mLightMatrices.mLightInv[shadowIndex] = glm::inverse(mLightMatrices.mLightSpace[shadowIndex]);
			}

			//the section of our paged shadowMap to render to 
			shadowState.mViewport = { page.x, page.y, page.width, page.height };
			scene.ForEach<TransformComponent, RenderComponent>([&](scene::GameObject obj)
			{
				const auto& render = obj.GetComponent<RenderComponent>();

				// reusing the perDrawConstantsBuffer for the model matrix slot
				PerDrawConstants draw;
				draw.u_model = mLightMatrices.mLightSpace[shadowIndex] * obj.GetWorldSpaceTransform();
				mEncoder->UpdateUniformBuffer(mPerDrawConstantsBuffer, &draw, sizeof(PerDrawConstants));

				mEncoder->DrawMesh(render.mesh, shadowState);
			});
		}
	});

	mEncoder->UpdateUniformBuffer(mLightMatricesBuffer, &mLightMatrices, sizeof(LightMatrices));
	mEncoder->UpdateUniformBuffer(mShadowPagesBuffer, &mShadowPages, sizeof(ShadowMapPages));
}

void RenderSystem::FillGBuffer(const Camera& camera, scene::Scene& scene)
{
	PushFrustumCull(scene, camera.GetProjectionMatrix() * camera.GetViewMatrix());

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

	PopFrustumCull(scene);
}

void RenderSystem::ResolveGBuffer(scene::Scene& scene)
{
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
	state.SetUniformBlock("LightSpaceMatrices_UBO", mLightMatricesBuffer);
	state.SetUniformBlock("ShadowPages_UBO", mShadowPagesBuffer);

	state.SetTexture("albedos", mGBuffer.AsTexture<OutputSlot::Color0>());
	state.SetTexture("normals", mGBuffer.AsTexture<OutputSlot::Color1>());
	state.SetTexture("coefficients", mGBuffer.AsTexture<OutputSlot::Color2>());
	state.SetTexture("depth", mGBuffer.AsTexture<OutputSlot::Depth>());

	state.SetTexture("shadowMap", mShadowMapFrameBuffer.AsTexture<OutputSlot::Depth>());

	mEncoder->DrawMesh(mFullscreenQuad, state);
}

void RenderSystem::DrawSkybox()
{
	RenderPass pass;
	pass.mName = "skybox";
	pass.mTarget = mHDRBuffer.mHandle;

	RenderState state;
	state.mDepthFunc = DepthFunction::LESS_EQUAL;
	state.mCullFace = CullFace::FRONT;
	state.mRenderPass = mEncoder->AddRenderPass(pass);
	state.mShader = mSkyboxShader;

	// Reuse PerDrawConstants so we dont need to declare another block
	// We will only use the model matrix entry
	PerDrawConstants cb;
	cb.u_model = mPerFrameConstants.u_proj * glm::mat4(glm::mat3(mPerFrameConstants.u_view));
	mEncoder->UpdateUniformBuffer(mPerDrawConstantsBuffer, &cb, sizeof(PerDrawConstants));

	state.SetUniformBlock("PerDrawConstants_UBO", mPerDrawConstantsBuffer);
	state.SetTexture("u_cubemap", mCubemap);

	mEncoder->DrawMesh(mCube, state);
}

void RenderSystem::Tonemap()
{
	RenderState state;
	state.mDepthFunc = DepthFunction::LESS_EQUAL;

	state.mAlphaBlendEnabled = false;
	state.mShader = mTonemapShader;
	state.mRenderPass = mEncoder->AddRenderPass("Tonemapping", ClearColor::YES, ClearDepth::NO);
	state.SetTexture("hdrBuffer", mHDRBuffer.AsTexture<OutputSlot::Color0>());

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
			fbDesc.Put<OutputSlot::Color0>(albedoDesc);

			TextureDescription2D normalDesc;
			normalDesc.mWidth = width;
			normalDesc.mHeight = height;
			normalDesc.mFormat = TextureFormat::RGB_FLOAT;
			fbDesc.Put<OutputSlot::Color1>(normalDesc);

			TextureDescription2D coeffDesc;
			coeffDesc.mWidth = width;
			coeffDesc.mHeight = height;
			coeffDesc.mFormat = TextureFormat::RGB_U8;
			fbDesc.Put<OutputSlot::Color2>(coeffDesc);

			TextureDescription2D depthDesc;
			depthDesc.mWidth = width;
			depthDesc.mHeight = height;
			depthDesc.mFormat = TextureFormat::DEPTH;
			fbDesc.Put<OutputSlot::Depth>(depthDesc);

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
			fbDesc.Put<OutputSlot::Color0>(colorDesc);

			TextureDescription2D depthDesc;
			depthDesc.mWidth = width;
			depthDesc.mHeight = height;
			depthDesc.mFormat = TextureFormat::DEPTH;
			fbDesc.Put<OutputSlot::Depth>(depthDesc);


			mHDRBuffer = mEncoder->CreateFrameBuffer(fbDesc);
		}
	}
}
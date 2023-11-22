#include "RenderSystem.h"

#include <core/Core.h>

#include <graphics/Vertex.h>
#include <graphics/MaterialManager.h>

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

	// Material buffer
	{
		auto& materialManager = Singletons::Get()->Resolve<MaterialManager>();

		auto& materials = materialManager->GetMaterials();
		u32 materialBufferSize = materials.size() * sizeof(graphics::Material);

		mMaterialBuffer = mEncoder->CreateUniformBuffer(materials.data(), materialBufferSize);
	}

	// Lighting data constants
	{
		mLightingBuffer = mEncoder->CreateUniformBuffer(nullptr, sizeof(LightBufferComponent::LightShaderBuffer));
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

	// Light bins
	{
		mLightBinsBuffer = mEncoder->CreateUniformBuffer(nullptr, sizeof(LightBins));
		mLightBinIndicesBuffer = mEncoder->CreateShaderBuffer(&mLightBinIndices, sizeof(u32) * LightBins::maxBinIndices);
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

	// update material buffer
	{
		auto materialManager = Singletons::Get()->Resolve<MaterialManager>();
		if (materialManager->CheckSetDirty())
		{
			auto& materials = materialManager->GetMaterials();
			u32 materialBufferSize = materials.size() * sizeof(graphics::Material);

			mMaterialBuffer = mEncoder->CreateUniformBuffer(materials.data(), materialBufferSize);
		}
	}

	ProcessPointLights(scene);

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

void RenderSystem::ProcessPointLights(scene::Scene& scene)
{
	// TODO (danielg): do this on GPU?

	LightBufferComponent* lightBuffer;
	scene.ForEach<LightBufferComponent>([&lightBuffer](scene::GameObject obj)
	{
		lightBuffer = &obj.GetComponent<LightBufferComponent>();
	});

	
	const i32 numPointLights = lightBuffer->lightBuffer.lightCounts.y;
	const auto& pointLights = lightBuffer->lightBuffer.pointLights;

	const i32 binSizeX = 64;
	const i32 binSizeY = 64;
	
	const i32 numBinsX = std::max((u32)(mResolution.x / binSizeX + 0.5f), 1u);
	const i32 numBinsY = std::max((u32)(mResolution.y / binSizeY + 0.5f), 1u);

	mLightBins.u_binsCounts.x = numBinsX;
	mLightBins.u_binsCounts.y = numBinsY;

	const i32 numBinsTotal = numBinsX * numBinsY;
	const i32 maxLights = numBinsTotal * LightBins::lightsPerBin;

	DEBUG_ASSERT(maxLights < LightBins::maxBinIndices, "Exceeded bin count");

	std::fill(std::begin(mLightBins.u_lightBins), std::end(mLightBins.u_lightBins), glm::vec4{0,0,0,0});

	// reset bins
	for (int i = 0; i < numBinsTotal; ++i)
	{
		mLightBins.u_lightBins[i].x = i * LightBins::lightsPerBin;
		mLightBins.u_lightBins[i].y = mLightBins.u_lightBins[i].x;
	}
	for (int i = 0; i < maxLights; ++i)
	{
		mLightBinIndices[i] = -1;
	}

	struct LightBounds
	{
		glm::vec2 min;
		glm::vec2 max;
	};

	const glm::mat4 view = mPerFrameConstants.u_view;
	const glm::mat4 proj = mPerFrameConstants.u_proj;
	const float zNear = proj[3][2] / (proj[2][2] - 1.0f);
	auto computeScreenBounds = [&view, &proj, zNear](const glm::vec3 pos, const float radius)
	{
		LightBounds bounds;
		bounds.min = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		bounds.max = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		std::array<glm::vec4,8> corners =
		{
			glm::vec4(pos.x + radius, pos.y + radius, pos.z + radius, 1.f),
			glm::vec4(pos.x + radius, pos.y + radius, pos.z - radius, 1.f),
			glm::vec4(pos.x + radius, pos.y - radius, pos.z + radius, 1.f),
			glm::vec4(pos.x + radius, pos.y - radius, pos.z - radius, 1.f),
			glm::vec4(pos.x - radius, pos.y + radius, pos.z + radius, 1.f),
			glm::vec4(pos.x - radius, pos.y + radius, pos.z - radius, 1.f),
			glm::vec4(pos.x - radius, pos.y - radius, pos.z + radius, 1.f),
			glm::vec4(pos.x - radius, pos.y - radius, pos.z - radius, 1.f)
		};

		for (auto& edge : corners)
		{
			glm::vec4 projPos = proj * view * edge;

			bounds.min.x = glm::min(bounds.min.x, projPos.x);
			bounds.min.y = glm::min(bounds.min.y, -projPos.y);
			bounds.max.x = glm::max(bounds.max.x, projPos.x);
			bounds.max.y = glm::max(bounds.max.y, -projPos.y);
		}
		
		return bounds;
	};

	u32 binnedLightCount = 0;
	for (i32 i = 0; i < numPointLights; ++i)
	{
		const auto& light = pointLights[i];

		LightBounds bounds = computeScreenBounds(light.position, light.params0.x * 2);
		
		i32 startX = glm::clamp((i32)(bounds.min.x * numBinsX), 0, numBinsX - 1);
		i32 endX   = glm::clamp((i32)(bounds.max.x * numBinsX), 0, numBinsX - 1);
		i32 startY = glm::clamp((i32)(bounds.min.y * numBinsY), 0, numBinsY - 1);
		i32 endY   = glm::clamp((i32)(bounds.max.y * numBinsY), 0, numBinsY - 1);

		for (i32 y = startY; y <= endY; ++y)
		{
			for (i32 x = startX; x <= endX; ++x)
			{
				i32 index = (y * numBinsX) + x;
				auto& bin = mLightBins.u_lightBins[index];
					
				if ((bin.y - bin.x) < LightBins::maxBinIndices)
				{
					mLightBinIndices[bin.y] = i;
					bin.y++;
					binnedLightCount++;
				}
				else
				{
					// NOTE (danielg): do something smarter than ignoring the extra lights?
					DEBUG_ASSERT(false, "Light bin full!");
				}
			}
		}
	}

	// compact bins
	/*{
		i32 nextAvailableStart = 0;
		for (i32 binIdx = 0; binIdx < numBinsTotal; ++binIdx)
		{
			auto& bin = mLightBins.u_lightBins[binIdx];

			i32 newBinStart = nextAvailableStart;

			for (i32 i = bin.x; i < bin.y; ++i)
			{
				mLightBins.u_lightBinIndices[nextAvailableStart] = mLightBins.u_lightBinIndices[i];
				nextAvailableStart++;
			}
			bin.x = newBinStart;
			bin.y = nextAvailableStart;
		}
		DEBUG_ASSERT(nextAvailableStart == binnedLightCount, "Light bin compaction logic is broken");
	}*/

	mEncoder->UpdateUniformBuffer(mLightBinsBuffer, &mLightBins, sizeof(LightBins), 0);
	mEncoder->UpdateShaderBuffer(mLightBinIndicesBuffer, &mLightBinIndices, sizeof(u32) * LightBins::maxBinIndices, 0);
}

void RenderSystem::FillShadowAtlas(scene::Scene& scene)
{
	bool bufferDirty = false;
	scene.ForEach<ShadowMapComponent>([&bufferDirty](scene::GameObject obj)
	{
		bufferDirty = bufferDirty || obj.GetComponent<ShadowMapComponent>().dirty;
	});
	if (!bufferDirty) return;
	

	RenderPass shadowPass;
	shadowPass.mName = "Shadow Atlas";
	shadowPass.mClearDepth = true;
	shadowPass.mTarget = mShadowMapFrameBuffer.mHandle;

	RenderState shadowState;
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

		// HACK: need to be able to clear individual pages before re-enabling
		//if (!shadow.dirty) return;
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
		if (shadowIndex >= LightBufferComponent::MAX_CASTERS) return;

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

	// Point lights
	scene.ForEach<PointLightComponent, ShadowMapComponent>([this, &shadowState, &scene](scene::GameObject& obj)
	{
		const auto& transform = obj.GetComponent<TransformComponent>();
		const auto& light = obj.GetComponent<PointLightComponent>();
		auto& shadow = obj.GetComponent<ShadowMapComponent>();

		const auto& pages = Singletons::Get()->Resolve<ShadowMapService>()->GetPages();

		// HACK: need to be able to clear individual pages before re-enabling
		//if (!shadow.dirty) return;
		shadow.dirty = false;

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
			if (shadowIndex >= LightBufferComponent::MAX_CASTERS) return;

			const auto& page = Singletons::Get()->Resolve<ShadowMapService>()->GetPage(shadowIndex);

			mShadowPages.mPage[shadowIndex] = { page.x, page.y, page.width, page.height };
			mShadowPages.mParams[shadowIndex].w = shadow.shadowMapBias[i];
			mShadowPages.mParams[shadowIndex].z = shadow.PCFSize + 1;

			mLightMatrices.mLightSpace[shadowIndex] = glm::perspective(shadow.FOV, shadow.aspect, shadow.nearPlane, shadow.farPlane) * lightViews[i];
			mLightMatrices.mLightInv[shadowIndex] = glm::inverse(mLightMatrices.mLightSpace[shadowIndex]);
			

			//the section of our paged shadowMap to render to 
			shadowState.mViewport = { page.x, page.y, page.width, page.height };

			PushFrustumCull(scene, mLightMatrices.mLightSpace[shadowIndex]);
			
			scene.ForEach<TransformComponent, RenderComponent>([&](scene::GameObject obj)
			{
				const auto& render = obj.GetComponent<RenderComponent>();

				// reusing the perDrawConstantsBuffer for the model matrix slot
				PerDrawConstants draw;
				draw.u_model = mLightMatrices.mLightSpace[shadowIndex] * obj.GetWorldSpaceTransform();
				mEncoder->UpdateUniformBuffer(mPerDrawConstantsBuffer, &draw, sizeof(PerDrawConstants));

				mEncoder->DrawMesh(render.mesh, shadowState);
			});
			
			PopFrustumCull(scene);
		}
	});

	mEncoder->UpdateUniformBuffer(mLightMatricesBuffer, &mLightMatrices, sizeof(LightMatrices));
	mEncoder->UpdateUniformBuffer(mShadowPagesBuffer, &mShadowPages, sizeof(ShadowMapPages));
}

void RenderSystem::FillGBuffer(const Camera& camera, scene::Scene& scene)
{
	auto materialManager = Singletons::Get()->Resolve<MaterialManager>();

	PushFrustumCull(scene, camera.GetProjectionMatrix() * camera.GetViewMatrix());

	//GBuffer fill
	uint8_t pass = mEncoder->AddRenderPass("GBuffer Fill", mGBuffer.mHandle, ClearColor::YES, ClearDepth::YES);
	scene.ForEach<TransformComponent, RenderComponent, NotFrustumCulledComponent>([&](const scene::GameObject obj)
	{
		const auto& render = obj.GetComponent<RenderComponent>();

		PerDrawConstants drawConstants =
		{
			obj.GetWorldSpaceTransform() ,
			render.material.idx,
		};
		mEncoder->UpdateUniformBuffer(mPerDrawConstantsBuffer, &drawConstants, sizeof(PerDrawConstants));

		RenderState state{};
		state.mRenderPass = pass;
		state.mAlphaBlendEnabled = false;
		state.mShader = mGBufferFillShader;
		state.SetUniformBlock("PerFrameConstants_UBO", mPerFrameContantsBuffer);
		state.SetUniformBlock("PerDrawConstants_UBO", mPerDrawConstantsBuffer);
		state.SetUniformBlock("Materials_UBO", mMaterialBuffer);

		auto material = materialManager->GetMaterial(render.material);

		// TODO (danielg): is this a weird way to store textures?
		if (material.mapFlags.x > 0) state.SetTexture("u_albedoMap",   { material.mapFlags.x });
		if (material.mapFlags.y > 0) state.SetTexture("u_normalMap",	{ material.mapFlags.y });
		if (material.mapFlags.z > 0) state.SetTexture("u_metallicMap",	{ material.mapFlags.z });
		if (material.mapFlags.w > 0) state.SetTexture("u_roughnessMap",	{ material.mapFlags.w });

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
	state.SetUniformBlock("LightBins_UBO", mLightBinsBuffer);
	state.SetStorageBlock("LightBinIndices_UBO", mLightBinIndicesBuffer);

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
	mResolution.x = width;
	mResolution.y = height;

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
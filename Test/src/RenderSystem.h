#pragma once

#include "scene/BaseComponents.h"
#include "scene/GameSystem.h"
#include "graphics/FrameEncoder.h"
#include "graphics/Texture.h"
#include "graphics/FrustumCuller.h"

#include "Components.h"

class RenderSystem : scene::GameSystem
{
public:
	static bool kReloadShaders;
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
		u32 materialHandle{};
		u32 pad[3];
	};
	graphics::UniformBufferHandle mPerDrawConstantsBuffer{};

	struct LightMatrices
	{
		glm::mat4 mLightSpace[LightBufferComponent::MAX_CASTERS];
		glm::mat4 mLightInv[LightBufferComponent::MAX_CASTERS];
	};
	LightMatrices mLightMatrices;
	graphics::UniformBufferHandle mLightMatricesBuffer{};
	
	struct ShadowMapPages
	{
		glm::vec4 mPage[LightBufferComponent::MAX_CASTERS];
		glm::vec4 mParams[LightBufferComponent::MAX_CASTERS]; // ?, ?, PCF filter size, bias
	};
	ShadowMapPages mShadowPages{};
	graphics::UniformBufferHandle mShadowPagesBuffer{};

	graphics::UniformBufferHandle mMaterialBuffer{};

	struct LightBins
	{
		static constexpr u32 lightsPerBin = 8;
		static constexpr u32 maxBins = (3840 / 32) * (3840 / 32);
		static constexpr u32 maxBinIndices = maxBins * lightsPerBin;
		
		glm::uvec4 u_binsCounts{}; //x,y,z, ?
		glm::uvec4 u_lightBins[maxBins]{}; //start, end, pad, pad;
	};

	
	LightBins mLightBins{};
	graphics::UniformBufferHandle mLightBinsBuffer{};

	glm::int32 mLightBinIndices[LightBins::maxBinIndices]{};
	graphics::ShaderBufferHandle mLightBinIndicesBuffer{};

	// holds LightBufferComponent::LightShaderBuffer
	graphics::UniformBufferHandle mLightingBuffer{};

	graphics::ShaderHandle mShadowAtlasFillShader{};
	graphics::ShaderHandle mGBufferFillShader{};
	graphics::ShaderHandle mGBufferResolveShader{};
	graphics::ShaderHandle mSkyboxShader{};
	graphics::ShaderHandle mTonemapShader{};

	graphics::FrameBuffer mGBuffer{};
	graphics::FrameBuffer mShadowMapFrameBuffer{};
	graphics::FrameBuffer mHDRBuffer{};

	graphics::MeshHandle mFullscreenQuad{};
	graphics::MeshHandle mCube{};

	graphics::TextureHandle mCubemap{};

	bool mFirstFrame = true;

	glm::uvec2 mResolution{};
		
	void InitRenderData(scene::Scene& scene);
	void ReloadShaders();

	void ProcessPointLights(scene::Scene& scene);
	void FillShadowAtlas(scene::Scene& scene);
	void FillGBuffer(const Camera& camera, scene::Scene& scene);
	void ResolveGBuffer(scene::Scene& scene);
	void DrawSkybox();
	void Tonemap();

public:
	
	RenderSystem()
	{

	}
	
	virtual ~RenderSystem()
	{

	}

	void ResizeGBuffer(int width, int height);

	void SetEncoder(gold::FrameEncoder* frameEncoder)
	{
		DEBUG_ASSERT(frameEncoder, "Frame Encoder should never be null!");
		mEncoder = frameEncoder;
	}

	virtual void Tick(scene::Scene& scene, float dt) override;
};
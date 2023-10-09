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

	graphics::FrameBuffer mGBuffer{};

	bool mFirstFrame = true;

	void InitRenderData(scene::Scene& scene);

	void FillGBuffer(const Camera& camera, scene::Scene& scene);

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

	const graphics::FrameBuffer& GetRenderTarget()
	{
		return mGBuffer;
	}

	virtual void Tick(scene::Scene& scene, float dt) override;
};
#pragma once 

#include "graphics/RenderTypes.h"
#include "scene/SceneGraph.h"
#include <graphics/FrameEncoder.h>
#include <scene/BaseComponents.h>

class LightBinning
{
private:
	struct LightBins
	{
		static constexpr u32 binSize = 32;
		static constexpr u32 lightsPerBin = 8;

		glm::uvec4 u_binsCounts{}; //x,y,z, ?
		std::vector<glm::uvec4> u_lightBins; //start, end, pad, pad;
	};

	LightBins mLightBins{};
	graphics::ShaderBufferHandle mLightBinsBuffer{};

	std::vector<glm::int32> mLightBinIndices{};
	graphics::ShaderBufferHandle mLightBinIndicesBuffer{};

public:

	void ProcessPointLights(scene::Scene& scene, const Camera& camera, u32 width, u32 height, gold::FrameEncoder& encoder);

	graphics::ShaderBufferHandle GetLightBins();
	graphics::ShaderBufferHandle getLightBinIndices();
};
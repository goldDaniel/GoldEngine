#include "LightBinning.h"


graphics::ShaderBufferHandle LightBinning::GetLightBins()
{
	return mLightBinsBuffer;
}

graphics::ShaderBufferHandle LightBinning::getLightBinIndices()
{
	return mLightBinIndicesBuffer;
}

void LightBinning::ProcessPointLights(scene::Scene& scene, const Camera& camera, u32 width, u32 height, gold::FrameEncoder& encoder)
{
	// TODO (danielg): do this on GPU? thread it?

	LightBufferComponent* lightBuffer;
	scene.ForEach<LightBufferComponent>([&lightBuffer](scene::GameObject obj)
		{
			lightBuffer = &obj.GetComponent<LightBufferComponent>();
		});


	const i32 numPointLights = lightBuffer->lightBuffer.lightCounts.y;
	const auto& pointLights = lightBuffer->lightBuffer.pointLights;

	const i32 binSizeX = LightBins::binSize;
	const i32 binSizeY = LightBins::binSize;

	const i32 numBinsX = std::max((u32)((float)width / binSizeX + 0.5f), 1u);
	const i32 numBinsY = std::max((u32)((float)height / binSizeY + 0.5f), 1u);

	const i32 numBinsTotal = numBinsX * numBinsY;
	const i32 maxLights = numBinsTotal * LightBins::lightsPerBin;


	mLightBins.u_binsCounts.x = numBinsX;
	mLightBins.u_binsCounts.y = numBinsY;

	// lazy init lightbins due to dynamic sizing 
	if (!mLightBinsBuffer.idx || !mLightBinIndicesBuffer.idx)
	{
		//+1 due to binCounts
		mLightBinsBuffer = encoder.CreateShaderBuffer(nullptr, sizeof(glm::uvec4) * (numBinsTotal + 1));
		mLightBinIndicesBuffer = encoder.CreateShaderBuffer(nullptr, maxLights * sizeof(i32));

		mLightBins.u_lightBins.resize(numBinsTotal);
		mLightBinIndices.resize(LightBins::lightsPerBin * numBinsTotal);
	}
	else if (numBinsTotal > mLightBins.u_lightBins.size() || maxLights > mLightBinIndices.size())
	{
		encoder.DestroyShaderBuffer(mLightBinsBuffer);
		encoder.DestroyShaderBuffer(mLightBinIndicesBuffer);

		//+1 due to binCounts
		mLightBinsBuffer = encoder.CreateShaderBuffer(nullptr, sizeof(glm::uvec4) * (numBinsTotal + 1));
		mLightBinIndicesBuffer = encoder.CreateShaderBuffer(nullptr, maxLights * sizeof(i32));

		mLightBins.u_lightBins.resize(numBinsTotal);
		mLightBinIndices.resize(LightBins::lightsPerBin * numBinsTotal);
	}

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

	const glm::mat4 view = camera.GetViewMatrix();
	const glm::mat4 proj = camera.GetProjectionMatrix();
	const float zNear = proj[3][2] / (proj[2][2] - 1.0f);
	auto computeScreenBounds = [&view, &proj, zNear](const glm::vec3 pos, const float radius)
		{
			LightBounds bounds;
			bounds.min = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
			bounds.max = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

			std::array<glm::vec4, 8> corners =
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

	i32 binnedLightCount = 0;
	for (i32 i = 0; i < numPointLights; ++i)
	{
		const auto& light = pointLights[i];

		LightBounds bounds = computeScreenBounds(light.position, static_cast<float>(light.params0.x));

		i32 startX = glm::clamp((i32)(bounds.min.x * numBinsX), 0, numBinsX - 1);
		i32 endX = glm::clamp((i32)(bounds.max.x * numBinsX), 0, numBinsX - 1);
		i32 startY = glm::clamp((i32)(bounds.min.y * numBinsY), 0, numBinsY - 1);
		i32 endY = glm::clamp((i32)(bounds.max.y * numBinsY), 0, numBinsY - 1);

		for (i32 y = startY; y <= endY; ++y)
		{
			for (i32 x = startX; x <= endX; ++x)
			{
				i32 index = (y * numBinsX) + x;
				auto& bin = mLightBins.u_lightBins[index];

				if ((bin.y - bin.x) < (u32)maxLights)
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
	{
		i32 nextAvailableStart = 0;
		for (i32 binIdx = 0; binIdx < numBinsTotal; ++binIdx)
		{
			auto& bin = mLightBins.u_lightBins[binIdx];

			i32 newBinStart = nextAvailableStart;

			for (i32 i = (i32)bin.x; i < (i32)bin.y; ++i)
			{
				mLightBinIndices[nextAvailableStart] = mLightBinIndices[i];
				nextAvailableStart++;
			}
			bin.x = newBinStart;
			bin.y = nextAvailableStart;
		}
		DEBUG_ASSERT(nextAvailableStart == binnedLightCount, "Light bin compaction logic is broken");
	}


	encoder.UpdateShaderBuffer(mLightBinsBuffer, &mLightBins.u_binsCounts, sizeof(glm::uvec4), 0);
	encoder.UpdateShaderBuffer(mLightBinsBuffer, &mLightBins.u_lightBins[0], sizeof(glm::uvec4) * numBinsTotal, sizeof(glm::uvec4));

	encoder.UpdateShaderBuffer(mLightBinIndicesBuffer, &mLightBinIndices[0], binnedLightCount * sizeof(i32), 0);
}
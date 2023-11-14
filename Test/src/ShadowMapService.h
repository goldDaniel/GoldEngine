#pragma once 

#include "graphics/Renderer.h"
#include <bitset>

using namespace graphics;

class ShadowMapService
{
public:
	struct ShadowPage
	{
		uint16_t x = 0;
		uint16_t y = 0;
		uint16_t width = 0;
		uint16_t height = 0;
		uint32_t shadowMapIndex = 0;
	};

	static constexpr uint16_t kTextureWidth = 8192*2;
	static constexpr uint16_t kTextureHeight = 8192;

	static constexpr uint16_t kPageWidth = 4096;
	static constexpr uint16_t kPageHeight = 4096;


	static constexpr uint32_t kNumPagesX = (kTextureWidth / kPageWidth);
	static constexpr uint32_t kNumPagesY = (kTextureHeight / kPageHeight);

	static constexpr uint32_t kNumPages = kNumPagesX * kNumPagesY;

private:
	FrameBuffer mShadowMap;

	std::vector<ShadowPage> mPages;

	uint16_t mLastX;
	uint16_t mLastY;
	std::bitset<kNumPages> mPageTable;

	uint32_t PageIndex(uint16_t x, uint16_t y);

	bool PageOccupied(uint16_t x, uint16_t y);

public:

	ShadowMapService();
	~ShadowMapService() = default;

	ShadowMapService(const ShadowMapService&) = delete;
	void operator=(const ShadowMapService&) = delete;

	void Reset();

	[[nodiscard]] 
	const ShadowPage& GetNextAvailablePageLocation();

	[[nodiscard]]
	const ShadowPage& GetPage(uint32_t index) const;

	[[nodiscard]] 
	const std::vector<ShadowPage>& GetPages() const;

	[[nodiscard]] 
	const FrameBuffer& GetFrameBuffer() const;
};
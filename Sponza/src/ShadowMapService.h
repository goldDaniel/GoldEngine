#pragma once 

#include "graphics/Renderer.h"
#include <bitset>

using namespace graphics;

class ShadowMapService
{
public:
	struct ShadowPage
	{
		u16 x = 0;
		u16 y = 0;
		u16 width = 0;
		u16 height = 0;
		u32 shadowMapIndex = 0;
	};

	static constexpr u16 kTextureWidth = 8192 * 2;
	static constexpr u16 kTextureHeight = 8192;

	static constexpr u16 kPageWidth = 2048;
	static constexpr u16 kPageHeight = 2048;

	static constexpr u32 kNumPagesX = (kTextureWidth / kPageWidth);
	static constexpr u32 kNumPagesY = (kTextureHeight / kPageHeight);

	static constexpr u32 kNumPages = kNumPagesX * kNumPagesY;

private:
	FrameBuffer mShadowMap;

	std::vector<ShadowPage> mPages;

	u16 mLastX;
	u16 mLastY;
	std::bitset<kNumPages> mPageTable;

	u32 PageIndex(u16 x, u16 y);

	bool PageOccupied(u16 x, u16 y);

public:

	ShadowMapService();
	~ShadowMapService() = default;

	ShadowMapService(const ShadowMapService&) = delete;
	void operator=(const ShadowMapService&) = delete;

	void Reset();

	[[nodiscard]] 
	const ShadowPage& GetNextAvailablePageLocation();

	[[nodiscard]]
	const ShadowPage& GetPage(u32 index) const;

	[[nodiscard]] 
	const std::vector<ShadowPage>& GetPages() const;

	[[nodiscard]] 
	const FrameBuffer& GetFrameBuffer() const;
};
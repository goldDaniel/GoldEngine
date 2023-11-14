#include "ShadowMapService.h"


ShadowMapService::ShadowMapService()
	: mLastX(0)
	, mLastY(0)
{
}

uint32_t ShadowMapService::PageIndex(uint16_t x, uint16_t y)
{
	uint32_t index = y * kNumPagesX + x;
	assert(index >= 0 && index < kNumPages);

	return index;
}

bool ShadowMapService::PageOccupied(uint16_t x, uint16_t y)
{
	assert(x >= 0 && x < (kNumPagesX));
	assert(y >= 0 && y < (kNumPagesY));

	uint32_t index = PageIndex(x, y);

	return mPageTable[index];
}

void ShadowMapService::Reset()
{
	mPageTable.reset();
	mLastX = 0;
	mLastY = 0;
}

const ShadowMapService::ShadowPage& ShadowMapService::GetNextAvailablePageLocation()
{
	assert(mLastY < kNumPagesY && "shadow buffer has been filled!");
	assert(!PageOccupied(mLastX, mLastY) && "page occupied! page selection logic broken");

	ShadowPage result;
	result.x = mLastX * kPageWidth;
	result.y = mLastY * kPageHeight;
	result.width = kPageWidth;
	result.height = kPageHeight;
	result.shadowMapIndex = PageIndex(mLastX, mLastY);

	mPages.push_back(result);

	if (mLastX < kNumPagesX - 1)
	{
		mLastX++;
	}
	else
	{
		mLastX = 0;
		mLastY++;
	}

	return mPages.back();
}

const ShadowMapService::ShadowPage& ShadowMapService::GetPage(uint32_t index) const
{
	for (const auto& page : mPages)
	{
		if (page.shadowMapIndex == index)
		{
			return page;
		}
	}

	assert(false && "Invalid shadow map index given!");
	return *(ShadowPage*)nullptr;
}

const std::vector<ShadowMapService::ShadowMapService::ShadowPage>& ShadowMapService::GetPages() const
{
	return mPages;
}

const graphics::FrameBuffer& ShadowMapService::GetFrameBuffer() const
{
	return mShadowMap;
}
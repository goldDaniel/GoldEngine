#include "ShadowMapService.h"

ShadowMapService::ShadowMapService()
	: mLastX(0)
	, mLastY(0)
{
}

u32 ShadowMapService::PageIndex(u16 x, u16 y)
{
	u32 index = y * kNumPagesX + x;
	DEBUG_ASSERT(index >= 0 && index < kNumPages, "Invalid page location!");

	return index;
}

bool ShadowMapService::PageOccupied(u16 x, u16 y)
{
	DEBUG_ASSERT(x >= 0 && x < (kNumPagesX), "Invalid page location!");
	DEBUG_ASSERT(y >= 0 && y < (kNumPagesY), "Invalid page location!");

	u32 index = PageIndex(x, y);

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
	DEBUG_ASSERT(mLastY < kNumPagesY, "shadow buffer has been filled!");
	DEBUG_ASSERT(!PageOccupied(mLastX, mLastY), "page occupied! page selection logic broken");

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

const ShadowMapService::ShadowPage& ShadowMapService::GetPage(u32 index) const
{
	for (const auto& page : mPages)
	{
		if (page.shadowMapIndex == index)
		{
			return page;
		}
	}

	DEBUG_ASSERT(false, "Invalid shadow map index given!");
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

#pragma once

#include "scene/GameSystem.h"
#include "graphics/FrameEncoder.h"

class RenderSystem : scene::GameSystem
{
private:
	gold::FrameEncoder* mEncoder;

public:
	
	RenderSystem()
	{

	}
	
	virtual ~RenderSystem()
	{

	}

	void SetEncoder(gold::FrameEncoder* frameEncoder)
	{
		DEBUG_ASSERT(frameEncoder, "Frame Encoder should never be null!");
		mEncoder = frameEncoder;
	}

	virtual void Tick(scene::Scene& scene, float dt) override
	{

	}
};
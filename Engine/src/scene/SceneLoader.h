#pragma once 

#include "core/Core.h"
#include "SceneGraph.h"
#include "graphics/FrameEncoder.h"

namespace scene
{
	class Loader
	{
	public:
		
		static void LoadGameObjectFromModel(Scene& scene, gold::FrameEncoder& encoder, const std::string& filepath);
	};
}
#pragma once 

#include "core/Core.h"
#include "SceneGraph.h"
#include "graphics/FrameEncoder.h"

namespace scene
{
	class Loader
	{
		void LoadGameObjectFromModel(Scene& scene, gold::FrameEncoder& encoder, std::string& filepath);
	};
}
#pragma once 

namespace scene
{
	class GameSystem
	{
	public:
		virtual ~GameSystem() = default;
		virtual void Tick(class Scene& scene, float dt) = 0;
	};
}


#pragma once 

#include "ImGuiWindow.h"

#include "scene/GameObject.h"
#include "scene/SceneGraph.h"

class PropertyWindow : public ImGuiWindow
{
private:
	std::vector<std::function<void(scene::GameObject)>> mDrawCallbacks;

	std::function<scene::GameObject(void)> mGetSelected;
	scene::Scene& mScene;

	template<typename T, typename DrawFunc>
	void DrawComponent(const std::string& name, scene::GameObject object, DrawFunc func);

public:
	PropertyWindow(scene::Scene& scene, std::function<scene::GameObject(void)>&& selectedFunc);

	template<typename Component>
	void AddComponentControl(const std::string& name, std::function<void(Component&)> draw)
	{
		mDrawCallbacks.push_back([=](scene::GameObject obj)
		{
			DrawComponent<Component>(name, obj, draw);
		});
	}

protected:
	virtual void DrawWindow(graphics::Renderer& renderer, gold::ServerResources& resources) override;
};
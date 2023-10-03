#pragma once 

#include "scene/SceneGraph.h"

#include "ImGuiWindow.h"

class SceneWindow : public ImGuiWindow
{
public:
	SceneWindow(scene::Scene* scene);

	void SetSelected(scene::GameObject obj);
	scene::GameObject GetSelected() const;

protected:
	virtual void DrawWindow(graphics::Renderer& renderer, gold::ServerResources& resources) override;
private:

	void _DrawNode(scene::GameObject obj);

	scene::GameObject mSelected;
	scene::Scene* mScene = nullptr;
};
#pragma once 

#include <core/Core.h>
#include <ui/ImGuiWindow.h>

#include <imgui.h>


struct RenderingToggles
{
	bool renderVoxelizedScene = false;
	int mipLevel = 0;

	bool doGlobalIllumination = true;
	bool cacheShadowMaps = true;
};

class RenderingTogglesWindow : public ImGuiWindow
{
public:
	RenderingTogglesWindow()
		: ImGuiWindow("Rendering Toggles", true)
	{

	}

protected:
	virtual void DrawWindow(graphics::Renderer& renderer, gold::ServerResources& resources) override
	{
		UNUSED_VAR(renderer);
		UNUSED_VAR(resources);

		auto toggles = Singletons::Get()->Resolve<RenderingToggles>();

		ImGui::Checkbox("Render Voxelized Scene", &toggles->renderVoxelizedScene);
		ImGui::SliderInt("Voxelized Scene MipMap Level", &toggles->mipLevel, 0, 10);
		ImGui::Separator();

		ImGui::Checkbox("Global Illumination Enabled", &toggles->doGlobalIllumination);
		ImGui::Separator();

		ImGui::Checkbox("ShadowMap Caching Enabled", &toggles->cacheShadowMaps);
		ImGui::Separator();
	}
};
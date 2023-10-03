#pragma once

#include "core/Core.h"

#include "ImGuiWindow.h"

#include <imgui.h>

class ViewportWindow : public ImGuiWindow
{
private:
	u32 mClientFrameBuffer = 0; 

public:
	ViewportWindow() : ImGuiWindow("Game Viewport", true)
	{
	}

	void SetTexture(u32 output)
	{
		mClientFrameBuffer = output;
	}

protected:
	virtual void StylePush() override
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	}

	virtual void StylePop() override
	{
		ImGui::PopStyleVar();
	}

	virtual void DrawWindow(graphics::Renderer& renderer, gold::ServerResources& resources) override
	{ 
		using namespace graphics;

		auto size = ImGui::GetWindowSize();
		size.x = glm::max(32.f, size.x);
		size.y = glm::max(32.f, size.y);
	
		TextureHandle& serverHandle = resources.get(TextureHandle{ mClientFrameBuffer });

		size.y -= 24;

		ImGui::BeginChild("Game Render");
		ImGui::Image((ImTextureID)(uint64_t)serverHandle.idx, size, { 0,1 }, { 1,0 });
		ImGui::EndChild();
	}
};
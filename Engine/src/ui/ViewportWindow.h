#pragma once

#include "core/Core.h"

#include "ImGuiWindow.h"

#include <imgui.h>

class ViewportWindow : public ImGuiWindow
{
private:
	u32 mViewportOutput = 0; 
	std::function<uint32_t(u32, u32)> mResizeFunc;

public:
	ViewportWindow(std::function<u32(u32, u32)>&& func)
		: ImGuiWindow("Game Viewport", true)
		, mResizeFunc(std::move(func))
	{

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

	virtual void DrawWindow() override
	{ 
		auto size = ImGui::GetWindowSize();
		size.x = glm::max(32.f, size.x);
		size.y = glm::max(32.f, size.y);

		mViewportOutput = mResizeFunc(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));

		size.y -= 24;

		ImGui::BeginChild("Game Render");
		ImGui::Image((ImTextureID)(uint64_t)mViewportOutput, size, { 0,1 }, { 1,0 });
		ImGui::EndChild();
	}
};
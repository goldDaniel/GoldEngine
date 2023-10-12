#include "ImGuiWindow.h"

#include <imgui.h>

ImGuiWindow::ImGuiWindow(const std::string& name, bool showWindow)
	:	mName(name)
	,	mShowWindow(showWindow)
	, mWindowFlags(ImGuiWindowFlags_NoCollapse)
{	
}

void ImGuiWindow::Show()
{
	mShowWindow = true;
}

void ImGuiWindow::Hide()
{
	mShowWindow = false;
}

void ImGuiWindow::Draw(graphics::Renderer& renderer, gold::ServerResources& resources)
{ 
	if (mShowWindow)
	{
		StylePush();
		ImGui::Begin(mName.c_str(), nullptr, mWindowFlags);

		auto size = ImGui::GetWindowSize();
		mSize.x = size.x;
		mSize.y = size.y;

		DrawWindow(renderer, resources);

		ImGui::End();
		StylePop();
	}	
}

void ImGuiWindow::SetFlags(int flags)
{
	mWindowFlags = flags | ImGuiWindowFlags_NoCollapse;
}

bool ImGuiWindow::IsShowing() const
{
	return mShowWindow;
}

const std::string& ImGuiWindow::GetName() const
{
	return mName;
}
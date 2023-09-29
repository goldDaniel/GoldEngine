#include "EditorUI.h"

#include <imgui.h>

using namespace gold;

void EditorUI::OnImguiRender()
{
	_DrawDockSpace();
	_DrawMainMenuBar();

	for (const auto& window : mWindows)
	{
		window->Draw();
	}

	// begin is called in _DrawDockSpace()
	ImGui::End();
}

void EditorUI::_DrawDockSpace()
{
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin("DockSpace", 0, window_flags);

	ImGui::PopStyleVar(2);

	// Submit the DockSpace
	ImGuiIO& io = ImGui::GetIO();

	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
}

void EditorUI::_DrawMainMenuBar()
{
	ImGui::BeginMainMenuBar();
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window"))
		{
			for (const auto& window : mWindows)
			{
				const std::string& name = window->GetName();
				bool isVisible = window->IsShowing();

				if (ImGui::MenuItem(name.c_str(), nullptr, &isVisible))
				{
					if (isVisible)
					{
						window->Show();
					}
					else
					{
						window->Hide();
					}
				}
			}

			ImGui::EndMenu();
		}

	}
	ImGui::EndMainMenuBar();
}


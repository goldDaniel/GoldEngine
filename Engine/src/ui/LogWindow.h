#pragma once

#include "EditorUI.h"

#include <imgui.h>

class LogWindow : public ImGuiWindow
{
public:
	LogWindow(bool showWindow = true)
		: ImGuiWindow("Log", showWindow)
	{
	}

protected:
	virtual void DrawWindow(graphics::Renderer& renderer, gold::ServerResources& resources) override
	{
		if (ImGui::Button("Clear Log"))
		{
			Singletons::Get()->Resolve<gold::Logging>()->ClearLog();
		}

		ImGui::BeginChild("LogScrollWindow");

		// by copy 
		const auto messages = Singletons::Get()->Resolve<gold::Logging>()->GetLogHistory();
		
		for (const auto& message : messages)
		{
			auto [level, msg] = message;
		
			bool stylePushed = true;
			switch (level)
			{
				case(spdlog::level::trace):
				{
					// white is OK 
					stylePushed = false;
					break;
				}
				case(spdlog::level::info):
				{
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
					break;
				}
				case(spdlog::level::warn):
				{
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(215, 195, 100, 255));
					break;
				}
				case(spdlog::level::err):
				{
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 105, 255));
					break;
				}
				case(spdlog::level::critical):
				{
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
					break;
				}
				default:
					stylePushed = false;
					DEBUG_ASSERT(false, "Invalid log level!");
			}
		
			ImGui::Text(msg.c_str());
		
			if (stylePushed)
			{
				ImGui::PopStyleColor();
			}

			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::EndChild();
	}
};
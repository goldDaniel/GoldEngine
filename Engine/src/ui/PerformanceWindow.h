#pragma once

class PerformanceWindow : public ImGuiWindow
{
public:
	PerformanceWindow(bool showWindow = false)
		: ImGuiWindow("Performance", showWindow)
	{
	}
protected:
	virtual void DrawWindow(graphics::Renderer& renderer, gold::ServerResources& resources) override
	{
		UNUSED_VAR(resources);

		const auto stats = renderer.GetPerfStats();
		
		u32 drawCalls = 0;
		u64 frameTimeNS = 0;
		for (u8 i = 0; i < stats.numPasses; ++i)
		{
			drawCalls += stats.mPassDrawCalls[i];
			frameTimeNS += stats.mPassTimeNS[i];	
		}

		if (ImGui::TreeNode("GPU Performance"))
		{
			std::string summaryText = "Totals:";
			summaryText += "\n- Draw Calls: " + std::to_string(drawCalls);
			summaryText += "\n- FrameTime(MS): " + std::to_string(frameTimeNS / 1000000.0);
			ImGui::Text(summaryText.c_str());

			ImGui::Separator();
			ImGui::Text("Render Passes");
			for (u8 i = 0; i < stats.numPasses; ++i)
			{
				std::string nodeName = stats.mPassNames[i] + "##_" + std::to_string(i);
				if (ImGui::TreeNode(nodeName.c_str()))
				{
					std::string text = "- Draw Calls: " + std::to_string(stats.mPassDrawCalls[i]);
					text += "\n- FrameTime(MS): " + std::to_string(stats.mPassTimeNS[i] / 1000000.0);
					ImGui::Text(text.c_str());
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}
};
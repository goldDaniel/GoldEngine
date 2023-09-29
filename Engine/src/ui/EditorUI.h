#pragma once

#include "core/Core.h"

#include "ImGuiWindow.h"

namespace gold
{
	class EditorUI
	{
	private:
		std::vector<std::unique_ptr<ImGuiWindow>> mWindows;

		void _DrawDockSpace();
		void _DrawMainMenuBar();
	public:
		template<typename T>
		T& AddEditorWindow(std::unique_ptr<T> window)
		{
			static_assert(std::is_base_of<ImGuiWindow, T>());

			auto& result = *window;
			mWindows.emplace_back(std::move(window));

			return result;
		}

		void OnImguiRender();
	};
}
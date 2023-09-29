#include "core/Core.h"

namespace gold
{
	class EditorUI
	{
	private:
		uint32_t mViewportOutput = 0;
		std::function<uint32_t(uint32_t, uint32_t)> mResizeFunc;

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
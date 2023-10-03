#pragma once 

#include "core/Core.h"

#include "graphics/Renderer.h"
#include "graphics/RenderResources.h"

class ImGuiWindow
{
private:
	bool mShowWindow = false;
	std::string mName;

	glm::ivec2 mSize{};

public:
	ImGuiWindow(const std::string& name, bool showWindow = false);
	virtual ~ImGuiWindow() {}

	void Show(); 
	void Hide(); 

	glm::ivec2 GetSize() const { return mSize; }

	bool IsShowing() const;

	void Draw(graphics::Renderer& renderer, gold::ServerResources& resources);

	const std::string& GetName() const;

protected:
	virtual void StylePush() {};
	virtual void StylePop() {};

	virtual void DrawWindow(graphics::Renderer& renderer, gold::ServerResources& resources) = 0;
};

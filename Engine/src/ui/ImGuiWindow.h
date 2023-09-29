#pragma once 

#include "core/Core.h"

class ImGuiWindow
{
private:
	bool mShowWindow = false;
	std::string mName;

public:
	ImGuiWindow(const std::string& name, bool showWindow = false);
	virtual ~ImGuiWindow() {}

	void Show(); 
	void Hide(); 

	bool IsShowing() const;

	void Draw();

	const std::string& GetName() const;

protected:
	virtual void StylePush() {};
	virtual void StylePop() {};

	virtual void DrawWindow() = 0;
};

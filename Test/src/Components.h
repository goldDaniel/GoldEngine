#pragma once

#include "core/Core.h"
#include "Camera.h"

class DebugCameraComponent
{
private:
	Camera mCamera;

public:
	glm::vec2 mPrevMouse;

	DebugCameraComponent(Camera camera = Camera());

	void ProcessMouseMovement(const glm::vec2& dir);
	void ProcessKeyboard(Camera_Movement dir, float dt);
	glm::mat4 GetViewMatrix() const;

	const glm::vec3& GetPosition() const
	{
		return mCamera.Position;
	}
};
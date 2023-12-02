#pragma once

#include "core/Core.h"
#include "scene/Camera.h"

class DebugCameraComponent
{
public:
	Camera mCamera{};
	glm::vec2 mPrevMouse{};

	DebugCameraComponent(Camera camera = Camera());

	void ProcessMouseMovement(const glm::vec2& dir);
	void ProcessKeyboard(Camera_Movement dir, float dt);
	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjectionMatrix() const;

	const glm::vec3& GetPosition() const
	{
		return mCamera.Position;
	}
};
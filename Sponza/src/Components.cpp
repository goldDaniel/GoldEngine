
#include "components.h"

DebugCameraComponent::DebugCameraComponent(Camera cam)
	: mCamera(cam) {}

void DebugCameraComponent::ProcessMouseMovement(const glm::vec2& dir)
{
	mCamera.ProcessMouseMovement(dir.x, dir.y);
}

void DebugCameraComponent::ProcessKeyboard(Camera_Movement dir, float dt)
{
	mCamera.ProcessKeyboard(dir, dt);
}

glm::mat4 DebugCameraComponent::GetProjectionMatrix() const
{
	return mCamera.GetProjectionMatrix();
}

glm::mat4 DebugCameraComponent::GetViewMatrix() const
{
	return mCamera.GetViewMatrix();
}
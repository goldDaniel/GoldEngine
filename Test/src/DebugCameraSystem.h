#pragma once

#include "scene/GameSystem.h"
#include "scene/SceneGraph.h"

#include "scene/BaseComponents.h"
#include "Components.h"

class DebugCameraSystem : public scene::GameSystem
{
public:
	virtual ~DebugCameraSystem() {}

	virtual void Tick(scene::Scene& scene, float dt) override
	{
		scene.ForEach<TransformComponent, DebugCameraComponent>([this, dt](scene::GameObject obj)
		{
			auto& transform = obj.GetComponent<TransformComponent>();
			auto& cam = obj.GetComponent<DebugCameraComponent>();

			auto& input = *Singletons::Get()->Resolve<gold::Input>();

			if (input.IsKeyDown(KeyCode::w))
			{
				cam.ProcessKeyboard(Camera_Movement::FORWARD, dt);
			}
			if (input.IsKeyDown(KeyCode::s))
			{
				cam.ProcessKeyboard(Camera_Movement::BACKWARD, dt);
			}
			if (input.IsKeyDown(KeyCode::a))
			{
				cam.ProcessKeyboard(Camera_Movement::LEFT, dt);
			}
			if (input.IsKeyDown(KeyCode::d))
			{
				cam.ProcessKeyboard(Camera_Movement::RIGHT, dt);
			}

			glm::vec2 mouseDelta = input.GetMousePos() - cam.mPrevMouse;
			cam.mPrevMouse = input.GetMousePos();

			if (input.IsButtonDown(MouseButton::LEFT))
			{
				cam.ProcessMouseMovement(mouseDelta);
			}

			transform.position = glm::inverse(cam.GetViewMatrix())[3];
		});
	}
};
#include "Input.h"

gold::Input::Input()
{
}

void gold::Input::Update()
{
    mKeyDownPrev = mKeyDown;
    mButtonDownPrev = mButtonDown;
}

bool gold::Input::IsKeyJustPressed(KeyCode code) const
{
    if (mKeyDown.find(code) != mKeyDown.end())
    {
        if (mKeyDownPrev.find(code) != mKeyDownPrev.end())
        {
            return mKeyDown.at(code) && !mKeyDownPrev.at(code);
        }

        return mKeyDown.at(code);
    }

    return false;
}

bool gold::Input::IsKeyDown(KeyCode code) const
{
    if (mKeyDown.find(code) != mKeyDown.end())
    {
        return mKeyDown.at(code);
    }
    return false;
}


bool gold::Input::IsButtonDown(MouseButton button) const
{
	if (mButtonDown.find(button) != mButtonDown.end())
	{
        return mButtonDown.at(button);
	}
    return false;
}

bool gold::Input::IsButtonJustPressed(MouseButton button) const
{
	if (mButtonDown.find(button) != mButtonDown.end())
	{
		if (mButtonDownPrev.find(button) != mButtonDownPrev.end())
		{
			return mButtonDown.at(button) && !mButtonDownPrev.at(button);
		}

		return mButtonDown.at(button);
	}

	return false;
}

const glm::vec2 gold::Input::GetMousePos() const
{
    return { mMouseX, mMouseY };
}

void gold::Input::SetKeyState(KeyCode code, bool state)
{
    mKeyDown[code] = state;
}

void gold::Input::SetMouseState(MouseButton button, bool state)
{
    mButtonDown[button] = state;
}

void gold::Input::SetMousePosition(int x, int y) { mMouseX = x; mMouseY = y; }
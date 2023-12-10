#pragma once

#include <fstream>
#include <sstream>
#include <functional>

namespace core
{
	class Scene;
	class GameObject;
}

namespace util
{
	std::string LoadStringFromFile(const std::string& filename);
	void WriteStringToFile(const std::string& filename, const std::string& data);


	uint32_t Hash(const void* data, size_t n, uint32_t hash = 2166136261u);

	struct Finally
	{
		std::function<void()> mAction;

		Finally(std::function<void()>&& action)
			: mAction(std::move(action))
		{

		}
		~Finally()
		{
			mAction();
		}
	};
}
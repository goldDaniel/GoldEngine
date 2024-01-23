#define SDL_MAIN_HANDLED

#include <core/Core.h>
#include <core/Logging.h>
#include "AssetProcessor.h"

#include <filesystem>

int main(int argc, const char** argv)
{
	Singletons::Get()->Register<gold::Logging>([]() { return std::make_shared<gold::Logging>(); });
	UNUSED_VAR(argc);

	if (argc != 3)
	{
		G_ERROR("Incorrect parameters.\nUsage: ./AssetProcessor \"inputFile\" \"outputFile\"");
		return 0;
	}

	const char* input = argv[1];
	const char* output = argv[2];

	if (!std::filesystem::exists(input))
	{
		G_ERROR("Cannot find input file: {}", input);
		return 0;
	}
	if (std::filesystem::is_directory(input))
	{
		G_ERROR("Input given is not a file, but a directory: {}", input);
		return 0;
	}

	std::string file(output);
	std::replace(file.begin(), file.end(), '\\', '/');
	std::string filepath = file.substr(0, file.find_last_of('/'));

	if (!std::filesystem::is_directory(filepath))
	{
		std::filesystem::create_directories(filepath);
	}

	assets::ProcessModelAsset(input, output);
	return 0;
}
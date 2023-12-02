#include "ShaderCompiler.h"

#include "core/Core.h"
#include "core/Util.h"
#include "Renderer.h"

using namespace graphics;

template<size_t N>
static constexpr size_t _length(char const(&)[N])
{
	return N - 1;
}

static bool _HandleIncludes(std::string& shaderSrc, const std::string& directory, const std::string& shaderName)
{
	UNUSED_VAR(shaderName);

	constexpr const char INCLUDE_KEY[] = "#include ";

	auto getAndValidatePathStart = [&](size_t includeEndIndex)
	{
		size_t startIdx = includeEndIndex + _length(INCLUDE_KEY);
		
		DEBUG_ASSERT(shaderSrc[startIdx] == '\"', "");
		if (shaderSrc[startIdx] != '\"')
		{
			//G_FATAL("Invalid Inlcude in shader: {}\n Expected \" at index{}", shaderName, startIdx);
			return std::numeric_limits<size_t>::max();
		}

		return startIdx;
	};
	
	auto getAndValidatePathEnd = [&](size_t includePathStart)
	{
		auto pathEnd = std::find(shaderSrc.begin() + includePathStart + 1, shaderSrc.end(), '\"');
		size_t endIdx = pathEnd - shaderSrc.begin();

		DEBUG_ASSERT(pathEnd != shaderSrc.end(), "");
		DEBUG_ASSERT(shaderSrc[endIdx] == '\"', "");
		if (pathEnd == shaderSrc.end())
		{
			//G_FATAL("Invalid Include in shader: {}\nCould not find closing \"", shaderName);
			return std::numeric_limits<size_t>::max();
		}

		return endIdx;
	};

	auto getAndValidateIncludeSrc = [&](size_t pathStart, size_t pathEnd)
	{
		// +1 to remove leading "
		std::string filename = shaderSrc.substr(pathStart + 1, pathEnd - pathStart - 1);

		std::string fileToInclude = directory + filename;
		return std::move(util::LoadStringFromFile(fileToInclude));
	};

	size_t includeIdx = shaderSrc.find(INCLUDE_KEY);
	while (includeIdx != std::string::npos)
	{
		size_t pathStart = getAndValidatePathStart(includeIdx);
		if (pathStart == std::numeric_limits<size_t>::max()) return false;

		size_t pathEnd = getAndValidatePathEnd(pathStart);
		if (pathEnd == std::numeric_limits<size_t>::max()) return false;

		DEBUG_ASSERT(pathStart != pathEnd, "");


		std::string includeSrc = getAndValidateIncludeSrc(pathStart, pathEnd);

		shaderSrc = shaderSrc.replace(includeIdx, (pathEnd - includeIdx + 1), includeSrc);
		includeIdx = shaderSrc.find(INCLUDE_KEY);
	} 

	return true;
}

std::string ShaderCompiler::CompileShader(const char* shaderPath)
{
	// get directory of shader
	std::string directory(shaderPath);
	size_t filenameStartIdx = directory.find_last_of('/');
	directory = directory.substr(0, filenameStartIdx + 1);

	std::string src = util::LoadStringFromFile(shaderPath);

	if (!_HandleIncludes(src, directory, shaderPath))
	{
		//G_FATAL("Shader Include failed! File: {}", shaderPath);
		DEBUG_ASSERT(false, "Shader Include failed!");
		return {};
	}

	return std::move(src);
}
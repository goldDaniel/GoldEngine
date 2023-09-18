#pragma once

#include <string>

namespace graphics
{
	class ShaderCompiler
	{
	public:
		static std::string CompileShader(const char* shaderPath);
	};
}
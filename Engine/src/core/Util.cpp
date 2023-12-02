#include "Util.h"

#include "Core.h"

std::string util::LoadStringFromFile(const std::string& filename)
{
	std::ifstream f(filename);
	std::string str;

	DEBUG_ASSERT(f, "file could not be opened: " + filename);

	std::ostringstream ss;
	ss << f.rdbuf();
	str = ss.str();
	
	return str;
}

void util::WriteStringToFile(const std::string& filename, const std::string& data)
{
	std::ofstream out(filename);
	out << data;
	out.close();

}

uint32_t util::Hash(const void* data, size_t n, uint32_t hash)
{
	const uint32_t prime = 16777619u;

	const char* cdata = (const char*)data;
	for (size_t i = 0; i < n; i++)
	{
		hash = (hash ^ cdata[i]) * prime;
	}
	return hash;
}

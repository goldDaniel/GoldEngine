#include "core/Core.h"

#include "memory/StackAllocator.h"
#include "memory/LinearAllocator.h"

#include <cstdlib>
#include <vector>
#include <iostream>


void PrintStats(gold::Allocator& alloc)
{
	const auto stats = alloc.GetStats();
	std::cout << "\n========================\n";
	std::cout << "Allocator Size: " << stats.mAllocatorSize << "\n";
	std::cout << "Allocator Used: " << stats.mUsed << "\n";
	std::cout << "Allocator Peak: " << stats.mPeak << "\n";
	std::cout << "========================\n";
}

int main(int argc, char** argv)
{
    std::string test = "";
    for (char i = 'a'; i <= 'z'; ++i)
    {
        test.push_back(i);
    }

    return 0;
}
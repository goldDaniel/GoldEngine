#include "core/Core.h"

#include "memory/StackAllocator.h"
#include "memory/LinearAllocator.h"
#include "memory/STLAllocator.h"

#include <cstdlib>
#include <vector>
#include <iostream>

struct Foo
{
    u32 a;
};

void PrintStats(gold::Allocator& allocator)
{
    const auto stats = allocator.GetStats();

    std::cout << "\n==========================\n";
    std::cout << "Allocator size: " << stats.mAllocatorSize << '\n';
    std::cout << "Allocator used: " << stats.mUsed << '\n';
    std::cout << "Allocator peak: " << stats.mPeak << '\n';
    std::cout << "\n==========================\n";
}

int main(int argc, char** argv)
{
    constexpr u64 MB = 1024 * 1024;
    constexpr u64 GB = 1024 * 1024 * 1024;

    gold::StackAllocator allocator(malloc(GB * 8), GB * 8);
    
	{
		std::vector<Foo, gold::STLAdapter<Foo>> vec(allocator);
        vec.reserve(2048);

        for (int i = 0; i < 2048; ++i)
        {
            vec.push_back(Foo{ static_cast<u32>(i) });
            PrintStats(allocator);
        }
    }

    return 0;
}
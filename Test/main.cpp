#include "core/Core.h"

#include "memory/StackAllocator.h"
#include "memory/LinearAllocator.h"
#include "memory/STLAllocator.h"

#include <cstdlib>
#include <vector>
#include <iostream>


struct Foo
{
    glm::vec3 a;
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
    constexpr u64 KB = 1024;
    constexpr u64 MB = KB * 1024;
    constexpr u64 GB = MB * 1024;

    gold::LinearAllocator allocator(malloc(GB * 1), GB * 1);

    return 0;
}
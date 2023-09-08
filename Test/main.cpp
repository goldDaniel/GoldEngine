#include "core/Core.h"

#include "memory/LinearAllocator.h"
#include "memory/STLAllocator.h"

#include <cstdlib>
#include <iostream>
#include <vector>

struct Foo
{
    u64 a;
    u64 b;
};

void PrintStats(gold::Allocator& allocator)
{

    auto stats = allocator.GetStats();
    std::cout << "\n====================\n";
    std::cout << "Total memory: " << stats.mAllocatorSize << "\n";
    std::cout << "in use: " << stats.mUsed << "\n";
    std::cout << "peak: " << stats.mPeak << "\n";
    std::cout << "====================\n";
}

int main(int argc, char** argv)
{
    constexpr u64 TWO_MB = 1024 * 1024 * 2;

    gold::LinearAllocator allocator(malloc(TWO_MB), TWO_MB);
    PrintStats(allocator);

    std::vector<Foo, gold::STLAdapter<Foo>> vec(allocator);

    return 0;
}
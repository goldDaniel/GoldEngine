#include "core/Core.h"

#include "memory/MemorySystem.h"

#include <cstdlib>
#include <vector>
#include <iostream>


int main(int argc, char** argv)
{
    gold::MemorySystem::Get().Init();

    std::string test = "";
    for (char i = 'a'; i <= 'z'; ++i)
    {
        test.push_back(i);
        std::cout << test << '\n';
    }

    return 0;
}
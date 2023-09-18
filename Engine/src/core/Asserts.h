#pragma once

#include <cassert>
#include <iostream>

#define DEBUG_ASSERT(cond, msg) \
	if(!(cond)) std::cout << msg << std::endl; \
	assert(cond)

#define STATIC_ASSERT(cond, msg) static_assert(cond, msg)
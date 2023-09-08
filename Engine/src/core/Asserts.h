#pragma once

#include <cassert>

#define DEBUG_ASSERT(cond) assert(cond)
#define STATIC_ASSERT(cond, msg) static_assert(cond, msg)
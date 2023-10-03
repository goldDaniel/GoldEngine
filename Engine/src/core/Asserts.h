#pragma once

#include <cassert>
#include "Logging.h"

#if defined(GOLD_ENGINE)
#define DEBUG_ASSERT(cond, ...) \
	if(!(cond)) G_ENGINE_FATAL(__VA_ARGS__); \
	assert(cond)
#else
#define DEBUG_ASSERT(cond, ...) \
	if(!(cond)) G_FATAL(__VA_ARGS__); \
	assert(cond)
#endif

#define STATIC_ASSERT(cond, msg) static_assert(cond, msg)
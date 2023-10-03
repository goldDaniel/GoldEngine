#pragma once

#include <cassert>
#include "Logging.h"

#if defined(GOLD_ENGINE)
#define DEBUG_ASSERT(cond, msg) \
	if(!(cond)) G_ENGINE_FATAL(msg); \
	assert(cond)
#else
#define DEBUG_ASSERT(cond, msg) \
	if(!(cond)) G_FATAL(msg); \
	assert(cond)
#endif

#define STATIC_ASSERT(cond, msg) static_assert(cond, msg)
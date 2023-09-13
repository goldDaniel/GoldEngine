#pragma once

#include "core/Core.h"

#include <glm/glm.hpp>

using u8	= unsigned char;
using u16	= unsigned short;
using u32	= unsigned int;
using u64	= unsigned long long;

STATIC_ASSERT(sizeof(u8)  == 1, "u8 size mismatch!");
STATIC_ASSERT(sizeof(u16) == 2, "u16 size mismatch!");
STATIC_ASSERT(sizeof(u32) == 4, "u32 size mismatch!");
STATIC_ASSERT(sizeof(u64) == 8, "u64 size mismatch!");

using i8	= char;
using i16	= short;
using i32	= int;
using i64	= long long;

STATIC_ASSERT(sizeof(i8)  == 1, "i8 size mismatch!");
STATIC_ASSERT(sizeof(i16) == 2, "i16 size mismatch!");
STATIC_ASSERT(sizeof(i32) == 4, "i32 size mismatch!");
STATIC_ASSERT(sizeof(i64) == 8, "i64 size mismatch!");

using f32	= float;
using f64	= double;

STATIC_ASSERT(sizeof(f32) == 4, "Type size mismatch!");
STATIC_ASSERT(sizeof(f64) == 8, "Type size mismatch!");

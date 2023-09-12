#pragma once

#include "core/Core.h"

#include <cstdint>
#include <glm/glm.hpp>

using u8	= uint8_t;
using u16	= uint16_t;
using u32	= uint32_t;
using u64	= uint64_t;

using i8	= int8_t;
using i16	= int16_t;
using i32	= int32_t;
using i64	= int64_t;

using f32	= float;
using f64	= double;

STATIC_ASSERT(sizeof(f32) == 4, "Type size mismatch!");
STATIC_ASSERT(sizeof(f64) == 8, "Type size mismatch!");

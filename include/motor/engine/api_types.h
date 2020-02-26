#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <motor/base/math_types.h>

// clang-format off
#if defined(_WIN32) && defined(_MSC_VER)
    #if defined(MT_ENGINE_BUILD_SHARED)
        #define MT_ENGINE_API __declspec(dllexport)
    #elif defined(MT_SHARED)
        #define MT_ENGINE_API __declspec(dllimport)
    #else
        #define MT_ENGINE_API
    #endif
#else
    #define MT_ENGINE_API 
#endif
// clang-format on

typedef struct MtStandardVertex
{
    Vec3 pos;
    Vec3 normal;
    Vec2 uv0;
    Vec4 tangent;
} MtStandardVertex;

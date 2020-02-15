#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// clang-format off
#if defined(_WIN32) && defined(_MSC_VER)
    #if defined(MT_GRAPHICS_BUILD_SHARED)
        #define MT_GRAPHICS_API __declspec(dllexport)
    #elif defined(MT_SHARED)
        #define MT_GRAPHICS_API __declspec(dllimport)
    #else
        #define MT_GRAPHICS_API
    #endif
#else
    #define MT_GRAPHICS_API 
#endif
// clang-format on

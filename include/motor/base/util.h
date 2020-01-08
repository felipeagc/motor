#pragma once

#include <stddef.h>

#if defined(_MSC_VER)
#define MT_ALIGNAS(x) __declspec(align(x))
#elif defined(__clang__)
#define MT_ALIGNAS(x) __attribute__((aligned(x)))
#elif defined(__GNUC__)
#define MT_ALIGNAS(x) __attribute__((aligned(x)))
#endif

#define MT_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

#define MT_MAX(a, b) (a > b) ? a : b
#define MT_MIN(a, b) (a > b) ? b : a

#define MT_INLINE static inline

// Returns the path's file extension, including the '.'
MT_INLINE const char *mt_path_ext(const char *path)
{
    const char *ret = "";
    do
    {
        if (*path == '.')
        {
            ret = path;
        }
    } while (*path++);
    return ret;
}

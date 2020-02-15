#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// clang-format off
#if defined(_MSC_VER)
    #define MT_ALIGNAS(x) __declspec(align(x))
#elif defined(__clang__)
    #define MT_ALIGNAS(x) __attribute__((aligned(x)))
#elif defined(__GNUC__)
    #define MT_ALIGNAS(x) __attribute__((aligned(x)))
#endif

#ifdef __GNUC__
    #define MT_PRINTF_FORMATTING(x, y) __attribute__((format(printf, x, y)))
#else
    #define MT_PRINTF_FORMATTING(x, y)
#endif
// clang-format on

#define MT_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

#define MT_MAX(a, b) (a > b) ? a : b
#define MT_MIN(a, b) (a > b) ? b : a

#define MT_INLINE static inline

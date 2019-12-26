#pragma once

#if defined(_MSC_VER)
#define MT_ALIGNAS(x) __declspec(align(x))
#elif defined(__clang__)
#define MT_ALIGNAS(x) __attribute__((aligned(x)))
#elif defined(__GNUC__)
#define MT_ALIGNAS(x) __attribute__((aligned(x)))
#endif

#define MT_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

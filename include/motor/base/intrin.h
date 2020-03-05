#pragma once

#include "api_types.h"

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#include <intrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t mt_popcount64(uint64_t n)
{
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    return __popcnt64(n);
#else
    return __builtin_popcountll(n);
#endif
}

#ifdef __cplusplus
}
#endif

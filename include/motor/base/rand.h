#pragma once

#include "api_types.h"

typedef struct MtXorShift
{
    uint64_t state;
} MtXorShift;

MT_BASE_API void mt_xor_shift_init(MtXorShift *xs, uint64_t seed);

MT_BASE_API uint64_t mt_xor_shift(MtXorShift *xs);

MT_BASE_API float mt_xor_shift_float(MtXorShift *xs, float min, float max);

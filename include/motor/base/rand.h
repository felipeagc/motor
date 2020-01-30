#pragma once

#include <stdint.h>

typedef struct MtXorShift
{
    uint64_t state;
} MtXorShift;

void mt_xor_shift_init(MtXorShift *xs, uint64_t seed);

uint64_t mt_xor_shift(MtXorShift *xs);

float mt_xor_shift_float(MtXorShift *xs, float min, float max);

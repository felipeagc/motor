#include <motor/base/rand.h>

void mt_xor_shift_init(MtXorShift *xs, uint64_t seed)
{
    xs->state = seed;
}

uint64_t mt_xor_shift(MtXorShift *xs)
{
    uint64_t x = xs->state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return xs->state = x;
}

float mt_xor_shift_float(MtXorShift *xs, float min, float max)
{
    float r = (float)mt_xor_shift(xs);
    r /= (float)UINT64_MAX;
    r *= (max - min);
    r += min;
    return r;
}

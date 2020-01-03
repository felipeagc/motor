#pragma once

#include <inttypes.h>
#include <string.h>

typedef struct MtAllocator MtAllocator;

#define MT_BITSET(size)                                                        \
    struct {                                                                   \
        uint8_t bytes[(size + 7) / 8];                                         \
    }

#define mt_bitset_get(bitset, index)                                           \
    ((bitset)->bytes[index / 8] & (1 << (index % 8)))

#define mt_bitset_enable(bitset, index)                                        \
    (bitset)->bytes[index / 8] |= (1 << (index % 8))

#define mt_bitset_disable(bitset, index)                                       \
    (bitset)->bytes[index / 8] &= ~(1 << (index % 8))

#define mt_static_bitset_clear(bitset)                                         \
    memset((bitset)->bytes, 0, sizeof((bitset)->bytes))

#define mt_dynamic_bitset_clear(bitset)                                        \
    memset((bitset)->bytes, 0, ((bitset)->nbits + 7) / 8)

typedef struct MtDynamicBitset {
    unsigned char *bytes;
    uint32_t nbits;
} MtDynamicBitset;

void mt_dynamic_bitset_init(
    MtDynamicBitset *bitset, uint32_t nbits, MtAllocator *alloc);

void mt_dynamic_bitset_destroy(MtDynamicBitset *bitset, MtAllocator *alloc);

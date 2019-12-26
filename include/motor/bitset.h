#ifndef MT_BITSET_H
#define MT_BITSET_H

#define MT_BITSET(size)                                                        \
  struct {                                                                     \
    unsigned char bytes[(size + 7) / 8];                                       \
  }

#define mt_bitset_get(bitset, index)                                           \
  ((bitset)->bytes[index / 8] & (1 << (index % 8)))

#define mt_bitset_enable(bitset, index)                                        \
  (bitset)->bytes[index / 8] |= (1 << (index % 8))

#define mt_bitset_disable(bitset, index)                                       \
  (bitset)->bytes[index / 8] &= ~(1 << (index % 8))

#define mt_bitset_clear_all(bitset, index)                                     \
  memset((bitset)->bytes, 0, sizeof((bitset)->bytes))

#endif

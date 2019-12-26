#include "../../include/motor/bitset.h"

#include "../../include/motor/arena.h"

void mt_dynamic_bitset_init(
    MtDynamicBitset *bitset, uint32_t nbits, MtArena *arena) {
    bitset->nbits = nbits;
    bitset->bytes = mt_alloc(arena, (bitset->nbits + 7) / 8);
    mt_dynamic_bitset_clear(bitset);
}

void mt_dynamic_bitset_destroy(MtDynamicBitset *bitset, MtArena *arena) {
    mt_free(arena, bitset->bytes);
}

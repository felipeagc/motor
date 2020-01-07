#include <motor/base/bitset.h>

#include <motor/base/allocator.h>

void mt_dynamic_bitset_init(MtDynamicBitset *bitset, uint32_t nbits, MtAllocator *alloc)
{
    bitset->nbits = nbits;
    bitset->bytes = mt_alloc(alloc, (bitset->nbits + 7) / 8);
    mt_dynamic_bitset_clear(bitset);
}

void mt_dynamic_bitset_destroy(MtDynamicBitset *bitset, MtAllocator *alloc)
{
    mt_free(alloc, bitset->bytes);
}

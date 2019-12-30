#pragma once

#include <stdint.h>

typedef struct MtAllocator MtAllocator;

void mt_bump_alloc_init(
    MtAllocator *alloc,
    uint64_t base_block_size,
    MtAllocator *parent_allocator);

void mt_bump_alloc_destroy(MtAllocator *alloc);

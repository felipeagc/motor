#pragma once

#include "api_types.h"

typedef struct MtAllocator MtAllocator;

MT_BASE_API void mt_bump_alloc_init(
    MtAllocator *alloc, uint64_t base_block_size, MtAllocator *parent_allocator);

MT_BASE_API void mt_bump_alloc_destroy(MtAllocator *alloc);

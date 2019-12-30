#pragma once

#include <stdint.h>

typedef struct MtAllocator MtAllocator;

void mt_arena_init(MtAllocator *alloc, uint64_t base_block_size);

void mt_arena_destroy(MtAllocator *alloc);

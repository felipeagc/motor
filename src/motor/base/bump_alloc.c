#include <motor/base/bump_alloc.h>
#include <motor/base/allocator.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct BumpBlock {
    uint8_t *data;
    uint64_t size;
    uint64_t pos;
    struct BumpBlock *next;
} BumpBlock;

typedef struct BumpAllocator {
    uint64_t block_size;
    uint64_t last_block_size;
    BumpBlock base_block;
    BumpBlock *last_block;
    MtAllocator *parent_allocator;
} BumpAllocator;

static void block_init(BumpAllocator *alloc, BumpBlock *block, uint64_t size) {
    block->data = mt_alloc(alloc->parent_allocator, size);
    block->size = size;
    block->pos  = 0;
    block->next = NULL;
}

static void block_destroy(BumpAllocator *alloc, BumpBlock *block) {
    if (block->next != NULL) {
        block_destroy(alloc, block->next);
        mt_free(alloc->parent_allocator, block->next);
        block->next = NULL;
    }

    mt_free(alloc->parent_allocator, block->data);
}

static void *block_alloc(BumpBlock *block, uint64_t size) {
    assert((block->size - block->pos) >= size);
    void *data = block->data + block->pos;
    block->pos += size;
    return data;
}

static void *bump_realloc(BumpAllocator *alloc, void *ptr, uint64_t size) {
    if (ptr == NULL && size > 0) {
        // Alloc

        uint64_t space = alloc->last_block->size - alloc->last_block->pos;
        if (space < size) {
            // Append new block
            alloc->last_block->next =
                mt_alloc(alloc->parent_allocator, sizeof(BumpBlock));
            alloc->last_block_size *= 2;
            alloc->last_block_size += size;
            block_init(alloc, alloc->last_block->next, alloc->last_block_size);
            alloc->last_block = alloc->last_block->next;
        }

        return block_alloc(alloc->last_block, size);
    }

    if (ptr != NULL && size > 0) {
        // Realloc
        assert(0);
    }

    return NULL;
}

void mt_bump_alloc_init(
    MtAllocator *alloc, uint64_t block_size, MtAllocator *parent_allocator) {
    BumpAllocator *bump = mt_alloc(parent_allocator, sizeof(BumpAllocator));
    memset(bump, 0, sizeof(*bump));

    bump->parent_allocator = parent_allocator;

    bump->block_size      = block_size;
    bump->last_block_size = bump->block_size;
    block_init(bump, &bump->base_block, block_size);
    bump->last_block = &bump->base_block;

    *alloc = (MtAllocator){
        .realloc = (void *)bump_realloc,
        .inst    = bump,
    };
}

void mt_bump_alloc_destroy(MtAllocator *alloc) {
    BumpAllocator *bump = (BumpAllocator *)alloc->inst;
    block_destroy(bump, &bump->base_block);
    mt_free(bump->parent_allocator, bump);
}

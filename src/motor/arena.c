#include "../../include/motor/arena.h"

#include "../../include/motor/util.h"
#include "../../include/motor/allocator.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MT_ARENA_ALIGNMENT 16
#define MT_ARENA_HEADER_ADDR(header) (((uint8_t *)header) + sizeof(AllocHeader))

typedef MT_ALIGNAS(MT_ARENA_ALIGNMENT) struct AllocHeader {
    uint64_t size;
    uint64_t used;
    struct AllocHeader *prev;
    struct AllocHeader *next;
} AllocHeader;

typedef struct ArenaBlock {
    uint8_t *storage;
    AllocHeader *first_header;
    struct ArenaBlock *next;
    struct ArenaBlock *prev;
} ArenaBlock;

typedef struct Arena {
    ArenaBlock *base_block;
    ArenaBlock *last_block;
    uint64_t base_block_size;
} Arena;

#ifndef MT_TEST

static void header_init(
    AllocHeader *header, uint64_t size, AllocHeader *prev, AllocHeader *next) {
    header->prev = prev;
    header->next = next;
    header->size = size;
    header->used = false;
}

static void header_merge_if_necessary(AllocHeader *header) {
    assert(header != NULL);
    assert(!header->used);

    if (header->prev != NULL && !header->prev->used) {
        header->prev->next = header->next;
        header->prev->size += header->size + sizeof(AllocHeader);
        header = header->prev;
    }

    if (header->next != NULL && !header->next->used) {
        header->size += header->next->size + sizeof(AllocHeader);
        header->next = header->next->next;
    }
}

static void
block_init(ArenaBlock *block, uint64_t block_size, ArenaBlock *prev) {
    assert(block_size > sizeof(AllocHeader));
    block->storage      = (uint8_t *)malloc(block_size);
    block->first_header = (AllocHeader *)block->storage;
    header_init(
        block->first_header, block_size - sizeof(AllocHeader), NULL, NULL);
    block->next = NULL;
    block->prev = prev;
    if (prev) {
        prev->next = block;
    }
}

static void block_destroy(ArenaBlock *block) {
    if (block == NULL) {
        return;
    }

    block_destroy(block->next);

    free(block->storage);
    free(block);
}

static void *arena_realloc(Arena *arena, void *ptr, uint64_t size) {
    if (ptr == NULL && size > 0) {
        // Alloc

        if (size > arena->base_block_size - sizeof(AllocHeader)) {
            arena->base_block_size *= 2;
            ArenaBlock *new_block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
            block_init(new_block, arena->base_block_size, arena->last_block);
            arena->last_block = new_block;

            return arena_realloc(arena, NULL, size);
        }

        AllocHeader *best_header   = NULL;
        ArenaBlock *block          = arena->last_block;
        bool can_insert_new_header = false;

        while (block != NULL) {
            AllocHeader *header = block->first_header;
            while (header != NULL) {
                if (!header->used && header->size >= size) {
                    best_header = header;

                    uint64_t padding = 0;
                    uint64_t uint64_to_pad =
                        ((uintptr_t)header) + sizeof(AllocHeader) + size;
                    if (uint64_to_pad % MT_ARENA_ALIGNMENT != 0) {
                        padding = MT_ARENA_ALIGNMENT -
                                  (uint64_to_pad % MT_ARENA_ALIGNMENT);
                    }

                    can_insert_new_header =
                        (header->size >= sizeof(AllocHeader) + size + padding);
                    break;
                }

                header = header->next;
            }

            if (best_header != NULL) {
                break;
            }

            block = block->prev;
        }

        if (best_header == NULL) {
            ArenaBlock *new_block = (ArenaBlock *)malloc(sizeof(ArenaBlock));
            block_init(new_block, arena->base_block_size, arena->last_block);
            arena->last_block = new_block;

            return arena_realloc(arena, NULL, size);
        }

        if (can_insert_new_header) {
            uint64_t padding = 0;
            uint64_t uint64_to_pad =
                ((uintptr_t)best_header) + sizeof(AllocHeader) + size;
            if (uint64_to_pad % MT_ARENA_ALIGNMENT != 0) {
                padding =
                    MT_ARENA_ALIGNMENT - (uint64_to_pad % MT_ARENA_ALIGNMENT);
            }

            // @NOTE: insert new header after the allocation
            uint64_t old_size = best_header->size;
            best_header->size = size + padding;
            AllocHeader *new_header =
            (AllocHeader
                 *)((uint8_t *)best_header + sizeof(AllocHeader) + best_header->size);
            header_init(
                new_header,
                old_size - best_header->size - sizeof(AllocHeader),
                best_header,
                best_header->next);
            best_header->next = new_header;
        }

        best_header->used = true;

        return MT_ARENA_HEADER_ADDR(best_header);
    }

    if (ptr != NULL && size > 0) {
        // Realloc

        AllocHeader *header =
            (AllocHeader *)(((uint8_t *)ptr) - sizeof(AllocHeader));

        if (header->size >= size) {
            // Already big enough
            header->used = true;
            return ptr;
        }

        AllocHeader *next_header = header->next;
        uint64_t next_sizes      = header->size;
        while (next_header != NULL && !next_header->used) {
            if (next_sizes >= size) {
                break;
            }
            next_sizes += sizeof(AllocHeader) + next_header->size;
            next_header = header->next;
        }

        if (next_sizes >= size) {
            // Grow header
            if (next_header->next != NULL) {
                next_header->next->prev = header;
            }
            header->next = next_header->next;
            header->size = next_sizes;
            header->used = true;

            return ptr;
        }

        void *new_ptr = arena_realloc(arena, NULL, size);
        memcpy(new_ptr, ptr, header->size);

        header->used = false;
        header_merge_if_necessary(header);

        return new_ptr;
    }

    if (ptr != NULL && size == 0) {
        // Free

        AllocHeader *header =
            (AllocHeader *)(((uint8_t *)ptr) - sizeof(AllocHeader));
        assert(MT_ARENA_HEADER_ADDR(header) == ptr);
        header->used = false;
        header_merge_if_necessary(header);
    }

    return NULL;
}

void mt_arena_init(MtAllocator *alloc, uint64_t base_block_size) {
    Arena *arena = malloc(sizeof(Arena));
    memset(arena, 0, sizeof(*arena));

    arena->base_block_size = base_block_size;

    arena->base_block = malloc(sizeof(ArenaBlock));
    memset(arena->base_block, 0, sizeof(ArenaBlock));

    block_init(arena->base_block, base_block_size, NULL);

    arena->last_block = arena->base_block;

    *alloc = (MtAllocator){
        .realloc = (void *)arena_realloc,
        .inst    = arena,
    };
}

void mt_arena_destroy(MtAllocator *alloc) {
    Arena *arena = (Arena *)alloc->inst;
    block_destroy(arena->base_block);
    free(arena);
}

#endif

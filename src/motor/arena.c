#include "../../include/motor/arena.h"

#include "../../include/motor/util.h"
#include "../../include/motor/allocator.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MT_ARENA_ALIGNMENT 16
#define MT_ARENA_HEADER_ADDR(header)                                           \
    (((uint8_t *)header) + sizeof(MtAllocHeader))

typedef MT_ALIGNAS(MT_ARENA_ALIGNMENT) struct MtAllocHeader {
    uint64_t size;
    uint64_t used;
    struct MtAllocHeader *prev;
    struct MtAllocHeader *next;
} MtAllocHeader;

typedef struct MtArenaBlock {
    uint8_t *storage;
    MtAllocHeader *first_header;
    struct MtArenaBlock *next;
    struct MtArenaBlock *prev;
} MtArenaBlock;

typedef struct MtArena {
    MtArenaBlock *base_block;
    MtArenaBlock *last_block;
    uint64_t base_block_size;
} MtArena;

#ifndef MT_TEST

static void header_init(
    MtAllocHeader *header,
    uint64_t size,
    MtAllocHeader *prev,
    MtAllocHeader *next) {
    header->prev = prev;
    header->next = next;
    header->size = size;
    header->used = false;
}

static void header_merge_if_necessary(MtAllocHeader *header) {
    assert(header != NULL);
    assert(!header->used);

    if (header->prev != NULL && !header->prev->used) {
        header->prev->next = header->next;
        header->prev->size += header->size + sizeof(MtAllocHeader);
        header = header->prev;
    }

    if (header->next != NULL && !header->next->used) {
        header->size += header->next->size + sizeof(MtAllocHeader);
        header->next = header->next->next;
    }
}

static void
block_init(MtArenaBlock *block, uint64_t block_size, MtArenaBlock *prev) {
    assert(block_size > sizeof(MtAllocHeader));
    block->storage      = (uint8_t *)malloc(block_size);
    block->first_header = (MtAllocHeader *)block->storage;
    header_init(
        block->first_header, block_size - sizeof(MtAllocHeader), NULL, NULL);
    block->next = NULL;
    block->prev = prev;
    if (prev) {
        prev->next = block;
    }
}

static void block_destroy(MtArenaBlock *block) {
    if (block == NULL) {
        return;
    }

    block_destroy(block->next);

    free(block->storage);
    free(block);
}

static void *arena_realloc(MtArena *arena, void *ptr, uint64_t size) {
    if (ptr == NULL && size > 0) {
        // Alloc

        if (size > arena->base_block_size - sizeof(MtAllocHeader)) {
            arena->base_block_size *= 2;
            MtArenaBlock *new_block =
                (MtArenaBlock *)malloc(sizeof(MtArenaBlock));
            block_init(new_block, arena->base_block_size, arena->last_block);
            arena->last_block = new_block;

            return arena_realloc(arena, NULL, size);
        }

        MtAllocHeader *best_header = NULL;
        MtArenaBlock *block        = arena->last_block;
        bool can_insert_new_header = false;

        while (block != NULL) {
            MtAllocHeader *header = block->first_header;
            while (header != NULL) {
                if (!header->used && header->size >= size) {
                    best_header = header;

                    uint64_t padding = 0;
                    uint64_t uint64_to_pad =
                        ((uintptr_t)header) + sizeof(MtAllocHeader) + size;
                    if (uint64_to_pad % MT_ARENA_ALIGNMENT != 0) {
                        padding = MT_ARENA_ALIGNMENT -
                                  (uint64_to_pad % MT_ARENA_ALIGNMENT);
                    }

                    can_insert_new_header =
                        (header->size >=
                         sizeof(MtAllocHeader) + size + padding);
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
            MtArenaBlock *new_block =
                (MtArenaBlock *)malloc(sizeof(MtArenaBlock));
            block_init(new_block, arena->base_block_size, arena->last_block);
            arena->last_block = new_block;

            return arena_realloc(arena, NULL, size);
        }

        if (can_insert_new_header) {
            uint64_t padding = 0;
            uint64_t uint64_to_pad =
                ((uintptr_t)best_header) + sizeof(MtAllocHeader) + size;
            if (uint64_to_pad % MT_ARENA_ALIGNMENT != 0) {
                padding =
                    MT_ARENA_ALIGNMENT - (uint64_to_pad % MT_ARENA_ALIGNMENT);
            }

            // @NOTE: insert new header after the allocation
            uint64_t old_size = best_header->size;
            best_header->size = size + padding;
            MtAllocHeader *new_header =
            (MtAllocHeader
                 *)((uint8_t *)best_header + sizeof(MtAllocHeader) + best_header->size);
            header_init(
                new_header,
                old_size - best_header->size - sizeof(MtAllocHeader),
                best_header,
                best_header->next);
            best_header->next = new_header;
        }

        best_header->used = true;

        return MT_ARENA_HEADER_ADDR(best_header);
    }

    if (ptr != NULL && size > 0) {
        // Realloc

        MtAllocHeader *header =
            (MtAllocHeader *)(((uint8_t *)ptr) - sizeof(MtAllocHeader));

        if (header->size >= size) {
            // Already big enough
            header->used = true;
            return ptr;
        }

        MtAllocHeader *next_header = header->next;
        uint64_t next_sizes        = header->size;
        while (next_header != NULL && !next_header->used) {
            if (next_sizes >= size) {
                break;
            }
            next_sizes += sizeof(MtAllocHeader) + next_header->size;
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

        MtAllocHeader *header =
            (MtAllocHeader *)(((uint8_t *)ptr) - sizeof(MtAllocHeader));
        assert(MT_ARENA_HEADER_ADDR(header) == ptr);
        header->used = false;
        header_merge_if_necessary(header);
    }

    return NULL;
}

void mt_arena_init(MtAllocator *alloc, uint64_t base_block_size) {
    MtArena *arena = malloc(sizeof(MtArena));
    memset(arena, 0, sizeof(*arena));

    arena->base_block_size = base_block_size;

    arena->base_block = malloc(sizeof(MtArenaBlock));
    memset(arena->base_block, 0, sizeof(MtArenaBlock));

    block_init(arena->base_block, base_block_size, NULL);

    arena->last_block = arena->base_block;

    *alloc = (MtAllocator){
        .realloc = (void *)arena_realloc,
        .inst    = arena,
    };
}

void mt_arena_destroy(MtAllocator *alloc) {
    MtArena *arena = (MtArena *)alloc->inst;
    block_destroy(arena->base_block);
    free(arena);
}

#endif

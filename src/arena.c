#include "arena.h"

#include "util.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MT_ARENA_ALIGNMENT 16
#define MT_ARENA_HEADER_ADDR(header)                                           \
  (((uint8_t *)header) + sizeof(MtAllocHeader))

typedef MT_ALIGNAS(MT_ARENA_ALIGNMENT) struct MtAllocHeader {
  uint32_t size;
  uint32_t used;
  uint32_t _pad1;
  uint32_t _pad2;
  struct MtAllocHeader *prev;
  struct MtAllocHeader *next;
} MtAllocHeader;

struct MtArenaBlock {
  uint8_t *storage;
  MtAllocHeader *first_header;
  MtArenaBlock *next;
  MtArenaBlock *prev;
};

#ifndef MT_TEST

static void header_init(
    MtAllocHeader *header,
    uint32_t size,
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
block_init(MtArenaBlock *block, uint32_t block_size, MtArenaBlock *prev) {
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

void mt_arena_init(MtArena *arena, uint32_t base_block_size) {
  memset(arena, 0, sizeof(*arena));

  arena->base_block_size = base_block_size;

  arena->base_block = malloc(sizeof(MtArenaBlock));
  memset(arena->base_block, 0, sizeof(MtArenaBlock));

  block_init(arena->base_block, base_block_size, NULL);

  arena->last_block = arena->base_block;
}

void mt_arena_destroy(MtArena *arena) { block_destroy(arena->base_block); }

void *mt_alloc(MtArena *arena, uint32_t size) {
  if (size > arena->base_block_size - sizeof(MtAllocHeader)) {
    arena->base_block_size *= 2;
    MtArenaBlock *new_block = (MtArenaBlock *)malloc(sizeof(MtArenaBlock));
    block_init(new_block, arena->base_block_size, arena->last_block);
    arena->last_block = new_block;

    return mt_alloc(arena, size);
  }

  MtAllocHeader *best_header = NULL;
  MtArenaBlock *block        = arena->last_block;
  bool can_insert_new_header = false;

  while (block != NULL) {
    MtAllocHeader *header = block->first_header;
    while (header != NULL) {
      if (!header->used && header->size >= size) {
        best_header = header;

        uint32_t padding = 0;
        uint32_t size_to_pad =
            ((uintptr_t)header) + sizeof(MtAllocHeader) + size;
        if (size_to_pad % MT_ARENA_ALIGNMENT != 0) {
          padding = MT_ARENA_ALIGNMENT - (size_to_pad % MT_ARENA_ALIGNMENT);
        }

        can_insert_new_header =
            (header->size >= sizeof(MtAllocHeader) + size + padding);
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
    MtArenaBlock *new_block = (MtArenaBlock *)malloc(sizeof(MtArenaBlock));
    block_init(new_block, arena->base_block_size, arena->last_block);
    arena->last_block = new_block;

    return mt_alloc(arena, size);
  }

  if (can_insert_new_header) {
    uint32_t padding = 0;
    uint32_t size_to_pad =
        ((uintptr_t)best_header) + sizeof(MtAllocHeader) + size;
    if (size_to_pad % MT_ARENA_ALIGNMENT != 0) {
      padding = MT_ARENA_ALIGNMENT - (size_to_pad % MT_ARENA_ALIGNMENT);
    }

    // @NOTE: insert new header after the allocation
    uint32_t old_size = best_header->size;
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

void *mt_calloc(MtArena *arena, uint32_t size) {
  void *ptr = mt_alloc(arena, size);
  if (ptr) {
    memset(ptr, 0, size);
  }
  return ptr;
}

void *mt_realloc(MtArena *arena, void *ptr, uint32_t size) {
  if (ptr == NULL) {
    return mt_alloc(arena, size);
  }

  MtAllocHeader *header =
      (MtAllocHeader *)(((uint8_t *)ptr) - sizeof(MtAllocHeader));

  if (header->size >= size) {
    // Already big enough
    header->used = true;
    return ptr;
  }

  MtAllocHeader *next_header = header->next;
  uint32_t next_sizes        = header->size;
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

  void *new_ptr = mt_alloc(arena, size);
  memcpy(new_ptr, ptr, header->size);

  header->used = false;
  header_merge_if_necessary(header);

  return new_ptr;
}

void mt_free(MtArena *arena, void *ptr) {
  MtAllocHeader *header =
      (MtAllocHeader *)(((uint8_t *)ptr) - sizeof(MtAllocHeader));
  assert(MT_ARENA_HEADER_ADDR(header) == ptr);
  header->used = false;
  header_merge_if_necessary(header);
}

#endif

#define MT_TEST
#include "arena.c"
#include "util.h"
#include <assert.h>
#include <stdio.h>

typedef MT_ALIGNAS(16) struct Vec4 { float v[4]; } Vec4;
typedef MT_ALIGNAS(16) struct Mat4 { float v[16]; } Mat4;

uint32_t header_count(MtArenaBlock *block) {
  uint32_t headers = 0;

  MtAllocHeader *header = block->first_header;
  while (header != NULL) {
    headers++;
    header = header->next;
  }

  return headers;
}

uint32_t block_count(MtArena *arena) {
  uint32_t blocks = 0;

  MtArenaBlock *block = arena->last_block;
  while (block != NULL) {
    blocks++;
    block = block->prev;
  }

  return blocks;
}

#define BLOCK_SIZE(alloc_size, count)                                          \
  ((sizeof(mt_alloc_header_t) + (alloc_size)) * count)

void test_create_destroy() {
  MtArena arena;
  mt_arena_init(&arena, 120);

  assert(header_count(arena.last_block) == 1);
  assert(block_count(&arena) == 1);

  mt_arena_destroy(&arena);
}

void test_alloc_simple() {
  MtArena arena;
  mt_arena_init(&arena, 120);

  assert(header_count(arena.last_block) == 1);
  assert(block_count(&arena) == 1);

  uint32_t *alloc1 = mt_alloc(&arena, sizeof(uint32_t));
  assert(alloc1 != NULL);
  *alloc1 = 32;

  mt_arena_destroy(&arena);
}

void test_alloc_aligned() {
  MtArena arena;
  mt_arena_init(&arena, 1 << 14);

  assert(header_count(arena.last_block) == 1);
  assert(block_count(&arena) == 1);

  {
    Vec4 *alloc = mt_alloc(&arena, sizeof(Vec4));
    assert(alloc != NULL);
    *alloc = (Vec4){{0.0, 0.0, 0.0, 0.0}};
  }

  {
    Vec4 *alloc = mt_alloc(&arena, sizeof(Vec4));
    assert(alloc != NULL);
    *alloc = (Vec4){{0.0, 0.0, 0.0, 0.0}};
  }

  {
    char *alloc = mt_alloc(&arena, sizeof(char));
    assert(alloc != NULL);
  }

  {
    Vec4 *alloc = mt_alloc(&arena, sizeof(Vec4));
    assert(alloc != NULL);
    *alloc = (Vec4){{0.0, 0.0, 0.0, 0.0}};
  }

  {
    char *alloc = mt_alloc(&arena, sizeof(char));
    assert(alloc != NULL);
  }

  {
    char *alloc = mt_alloc(&arena, sizeof(char));
    assert(alloc != NULL);
  }

  {
    Mat4 *alloc = mt_alloc(&arena, sizeof(Mat4));
    assert(alloc != NULL);
    *alloc = (Mat4){0};
  }

  {
    Vec4 *alloc = mt_alloc(&arena, sizeof(Vec4));
    assert(alloc != NULL);
    *alloc = (Vec4){{0.0, 0.0, 0.0, 0.0}};
  }

  mt_arena_destroy(&arena);
}

void test_alloc_multi_block() {
  MtArena arena;

  uint32_t per_block = 3;
  size_t alloc_size  = sizeof(MtAllocHeader) * 2;

  mt_arena_init(&arena, (sizeof(MtAllocHeader) + alloc_size) * per_block);

  for (uint32_t i = 0; i < per_block * 20; i++) {
    uint64_t *alloc = mt_alloc(&arena, alloc_size);
    assert(alloc != NULL);
    assert(block_count(&arena) == ((i / per_block) + 1));
  }

  mt_arena_destroy(&arena);
}

void test_alloc_too_big() {
  MtArena arena;

  mt_arena_init(&arena, 160);

  {
    uint64_t *alloc = mt_alloc(&arena, 160);
    assert(alloc != NULL);
    *alloc = 123;
  }

  {
    uint64_t *alloc = mt_alloc(&arena, 10000);
    assert(alloc != NULL);
    *alloc = 123;
  }

  {
    uint64_t *alloc =
        mt_alloc(&arena, arena.base_block_size - sizeof(MtAllocHeader));
    assert(alloc != NULL);
  }

  mt_arena_destroy(&arena);
}

void test_alloc_free() {
  MtArena arena;

  mt_arena_init(&arena, 160);

  {
    void *alloc1 = mt_alloc(&arena, arena.base_block_size - sizeof(MtAllocHeader));
    assert(alloc1 != NULL);
    mt_free(&arena, alloc1);

    void *alloc2 = mt_alloc(&arena, arena.base_block_size - sizeof(MtAllocHeader));
    assert(alloc2 != NULL);

    assert(alloc1 == alloc2);
  }

  mt_arena_destroy(&arena);
}

void test_alloc_realloc_grow() {
  MtArena arena;

  mt_arena_init(&arena, 160);

  {
    uint32_t *alloc1 = mt_alloc(&arena, sizeof(uint32_t));
    assert(alloc1 != NULL);
    assert(block_count(&arena) == 1);

    uint32_t *alloc2 = mt_realloc(&arena, alloc1, sizeof(uint32_t) * 4);
    assert(alloc2 != NULL);
    assert(alloc2 == alloc1);
    assert(block_count(&arena) == 1);

    uint32_t *alloc3 =
        mt_realloc(&arena, alloc2, arena.base_block_size - sizeof(MtAllocHeader));
    assert(alloc3 != NULL);
    assert(alloc3 == alloc2);
    assert(block_count(&arena) == 1);
  }

  mt_arena_destroy(&arena);
}

void test_alloc_realloc_fragmented() {
  MtArena arena;

  mt_arena_init(&arena, 160);

  {
    uint32_t *alloc1 = mt_alloc(&arena, sizeof(uint32_t));
    assert(alloc1 != NULL);
    *alloc1 = 3;

    void *alloc2 = mt_alloc(&arena, 4);
    assert(alloc2 != NULL);

    uint32_t *alloc3 = mt_realloc(&arena, alloc1, MT_ARENA_ALIGNMENT + 1);
    assert(alloc3 != NULL);
    assert(alloc3 != alloc1);
    assert(*alloc3 == 3);
  }

  mt_arena_destroy(&arena);
}

int main() {
  test_create_destroy();
  test_alloc_simple();
  test_alloc_aligned();
  test_alloc_multi_block();
  test_alloc_too_big();
  test_alloc_free();
  test_alloc_realloc_grow();
  test_alloc_realloc_fragmented();

  printf("Success\n");

  return 0;
}

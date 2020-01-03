#include <motor/base/allocator.h>
#include <motor/base/arena.h>
#include <motor/base/util.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

typedef MT_ALIGNAS(16) struct Vec4 { float v[4]; } Vec4;
typedef MT_ALIGNAS(16) struct Mat4 { float v[16]; } Mat4;

#define MT_ARENA_ALIGNMENT 16
#define MT_ARENA_HEADER_ADDR(header)                                           \
    (((uint8_t *)header) + sizeof(MtAllocHeader))

typedef MT_ALIGNAS(MT_ARENA_ALIGNMENT) struct MtAllocHeader {
    size_t size;
    size_t used;
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
    size_t base_block_size;
} MtArena;

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
    MtAllocator alloc;
    mt_arena_init(&alloc, 120);

    MtArena *arena = (MtArena *)alloc.inst;

    assert(header_count(arena->last_block) == 1);
    assert(block_count(arena) == 1);

    mt_arena_destroy(&alloc);
}

void test_alloc_simple() {
    MtAllocator alloc;
    mt_arena_init(&alloc, 120);

    MtArena *arena = (MtArena *)alloc.inst;

    assert(header_count(arena->last_block) == 1);
    assert(block_count(arena) == 1);

    uint32_t *alloc1 = mt_alloc(&alloc, sizeof(uint32_t));
    assert(alloc1 != NULL);
    *alloc1 = 32;

    mt_arena_destroy(&alloc);
}

void test_alloc_aligned() {
    MtAllocator alloc;
    mt_arena_init(&alloc, 1 << 14);

    MtArena *arena = (MtArena *)alloc.inst;

    assert(header_count(arena->last_block) == 1);
    assert(block_count(arena) == 1);

    {
        Vec4 *a = mt_alloc(&alloc, sizeof(Vec4));
        assert(a != NULL);
        *a = (Vec4){{0.0, 0.0, 0.0, 0.0}};
    }

    {
        Vec4 *a = mt_alloc(&alloc, sizeof(Vec4));
        assert(a != NULL);
        *a = (Vec4){{0.0, 0.0, 0.0, 0.0}};
    }

    {
        char *a = mt_alloc(&alloc, sizeof(char));
        assert(a != NULL);
    }

    {
        Vec4 *a = mt_alloc(&alloc, sizeof(Vec4));
        assert(a != NULL);
        *a = (Vec4){{0.0, 0.0, 0.0, 0.0}};
    }

    {
        char *a = mt_alloc(&alloc, sizeof(char));
        assert(a != NULL);
    }

    {
        char *a = mt_alloc(&alloc, sizeof(char));
        assert(a != NULL);
    }

    {
        Mat4 *a = mt_alloc(&alloc, sizeof(Mat4));
        assert(a != NULL);
        *a = (Mat4){0};
    }

    {
        Vec4 *a = mt_alloc(&alloc, sizeof(Vec4));
        assert(a != NULL);
        *a = (Vec4){{0.0, 0.0, 0.0, 0.0}};
    }

    mt_arena_destroy(&alloc);
}

void test_alloc_multi_block() {
    MtAllocator alloc;

    uint32_t per_block = 3;
    size_t alloc_size  = sizeof(MtAllocHeader) * 2;

    mt_arena_init(&alloc, (sizeof(MtAllocHeader) + alloc_size) * per_block);

    MtArena *arena = (MtArena *)alloc.inst;

    for (uint32_t i = 0; i < per_block * 20; i++) {
        uint64_t *a = mt_alloc(&alloc, alloc_size);
        assert(a != NULL);
        assert(block_count(arena) == ((i / per_block) + 1));
    }

    mt_arena_destroy(&alloc);
}

void test_alloc_too_big() {
    MtAllocator alloc;
    mt_arena_init(&alloc, 160);

    MtArena *arena = (MtArena *)alloc.inst;

    {
        uint64_t *a = mt_alloc(&alloc, 160);
        assert(a != NULL);
        *a = 123;
    }

    {
        uint64_t *a = mt_alloc(&alloc, 10000);
        assert(a != NULL);
        *a = 123;
    }

    {
        uint64_t *a =
            mt_alloc(&alloc, arena->base_block_size - sizeof(MtAllocHeader));
        assert(a != NULL);
    }

    mt_arena_destroy(&alloc);
}

void test_alloc_free() {
    MtAllocator alloc;
    mt_arena_init(&alloc, 160);

    MtArena *arena = (MtArena *)alloc.inst;

    {
        void *alloc1 =
            mt_alloc(&alloc, arena->base_block_size - sizeof(MtAllocHeader));
        assert(alloc1 != NULL);
        mt_free(&alloc, alloc1);

        void *alloc2 =
            mt_alloc(&alloc, arena->base_block_size - sizeof(MtAllocHeader));
        assert(alloc2 != NULL);

        assert(alloc1 == alloc2);
    }

    mt_arena_destroy(&alloc);
}

void test_alloc_realloc_grow() {
    MtAllocator alloc;
    mt_arena_init(&alloc, 160);

    MtArena *arena = (MtArena *)alloc.inst;

    {
        uint32_t *alloc1 = mt_alloc(&alloc, sizeof(uint32_t));
        assert(alloc1 != NULL);
        assert(block_count(arena) == 1);

        uint32_t *alloc2 = mt_realloc(&alloc, alloc1, sizeof(uint32_t) * 4);
        assert(alloc2 != NULL);
        assert(alloc2 == alloc1);
        assert(block_count(arena) == 1);

        uint32_t *alloc3 = mt_realloc(
            &alloc, alloc2, arena->base_block_size - sizeof(MtAllocHeader));
        assert(alloc3 != NULL);
        assert(alloc3 == alloc2);
        assert(block_count(arena) == 1);
    }

    mt_arena_destroy(&alloc);
}

void test_alloc_realloc_fragmented() {
    MtAllocator alloc;
    mt_arena_init(&alloc, 160);

    MtArena *arena = (MtArena *)alloc.inst;

    {
        uint32_t *alloc1 = mt_alloc(&alloc, sizeof(uint32_t));
        assert(alloc1 != NULL);
        *alloc1 = 3;

        void *alloc2 = mt_alloc(&alloc, 4);
        assert(alloc2 != NULL);

        uint32_t *alloc3 = mt_realloc(&alloc, alloc1, MT_ARENA_ALIGNMENT + 1);
        assert(alloc3 != NULL);
        assert(alloc3 != alloc1);
        assert(*alloc3 == 3);
    }

    mt_arena_destroy(&alloc);
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

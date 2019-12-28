#include <motor/hashmap.h>
#include <motor/arena.h>
#include <motor/array.h>
#include <assert.h>
#include <stdio.h>

void test1(MtArena *arena) {
    MtHashMap h;
    mt_hash_init(&h, 2, arena);

    assert(mt_hash_set_uint(&h, 0, 123) == 123);
    assert(mt_hash_set_uint(&h, 1, 321) == 321);
    assert(mt_hash_set_uint(&h, 2, 456) == 456);
    assert(mt_hash_set_uint(&h, 3, 90) == 90);

    assert(mt_hash_get_uint(&h, 0) == 123);
    assert(mt_hash_get_uint(&h, 1) == 321);
    assert(mt_hash_get_uint(&h, 2) == 456);
    assert(mt_hash_get_uint(&h, 3) == 90);

    mt_hash_remove(&h, 0);
    assert(mt_hash_get_uint(&h, 0) == MT_HASH_NOT_FOUND);

    assert(mt_hash_set_uint(&h, 2, 456) == 456);
    assert(mt_hash_get_uint(&h, 2) == 456);

    mt_hash_destroy(&h);
}

void test2(MtArena *arena) {
    MtHashMap h;
    mt_hash_init(&h, 13, arena);

    typedef struct Struct {
        int32_t a;
    } Struct;

    Struct *data = NULL;
    mt_array_reserve(arena, data, 123);
    assert(mt_array_size(data) == 0);
    assert(mt_array_capacity(data) == 123);

    Struct s = {.a = 123};
    int i;
    assert((i = (mt_array_push(arena, data, s) - data)) == 0);
    assert(mt_hash_set_uint(&h, s.a, i) != MT_HASH_NOT_FOUND);

    assert(mt_hash_set_uint(&h, 1, 321) == 321);

    mt_array_free(arena, data);

    mt_hash_destroy(&h);
}

int main() {
    MtArena arena;
    mt_arena_init(&arena, 1 << 14);

    test1(&arena);
    test2(&arena);

    mt_arena_destroy(&arena);

    printf("Success\n");

    return 0;
}

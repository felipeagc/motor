#include <motor/hashmap.h>
#include <motor/arena.h>
#include <motor/allocator.h>
#include <motor/array.h>
#include <assert.h>
#include <stdio.h>

void test1(MtAllocator *alloc) {
    MtHashMap h;
    mt_hash_init(&h, 2, alloc);

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

void test2(MtAllocator *alloc) {
    MtHashMap h;
    mt_hash_init(&h, 13, alloc);

    typedef struct Struct {
        int32_t a;
    } Struct;

    Struct *data = NULL;
    mt_array_reserve(alloc, data, 123);
    assert(mt_array_size(data) == 0);
    assert(mt_array_capacity(data) == 123);

    Struct s = {.a = 123};
    int i;
    assert((i = (mt_array_push(alloc, data, s) - data)) == 0);
    assert(mt_hash_set_uint(&h, s.a, i) != MT_HASH_NOT_FOUND);

    assert(mt_hash_set_uint(&h, 1, 321) == 321);

    mt_array_free(alloc, data);

    mt_hash_destroy(&h);
}

int main() {
    MtAllocator alloc;
    mt_arena_init(&alloc, 1 << 14);

    test1(&alloc);
    test2(&alloc);

    mt_arena_destroy(&alloc);

    printf("Success\n");

    return 0;
}

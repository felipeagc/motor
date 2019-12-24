#include <motor/hashmap.h>
#include <motor/arena.h>
#include <motor/array.h>
#include <assert.h>
#include <stdio.h>

void test1(MtArena *arena) {
  MtHashMap h;
  mt_hash_init(&h, 2, arena);

  assert(mt_hash_set(&h, 0, 123) == 123);
  assert(mt_hash_set(&h, 1, 321) == 321);
  assert(mt_hash_set(&h, 2, 456) == MT_HASH_NOT_FOUND);
  assert(mt_hash_set(&h, 3, 90) == MT_HASH_NOT_FOUND);

  assert(mt_hash_get(&h, 0) == 123);
  assert(mt_hash_get(&h, 1) == 321);
  assert(mt_hash_get(&h, 2) == MT_HASH_NOT_FOUND);
  assert(mt_hash_get(&h, 3) == MT_HASH_NOT_FOUND);

  mt_hash_remove(&h, 0);
  assert(mt_hash_get(&h, 0) == MT_HASH_NOT_FOUND);

  assert(mt_hash_set(&h, 2, 456) == 456);
  assert(mt_hash_get(&h, 2) == 456);

  mt_hash_destroy(&h);
}

void test2(MtArena *arena) {
  MtHashMap h;
  mt_hash_init(&h, 13, arena);

  typedef struct Struct {
    int32_t a;
  } Struct;

  Struct *data = NULL;
  mt_array_reserve(data, 123);
  assert(mt_array_size(data) == 0);
  assert(mt_array_capacity(data) == 123);

  Struct s = {.a = 123};
  int i;
  assert((i = mt_array_push(data, s)) == 0);
  assert(mt_hash_set(&h, s.a, i) != MT_HASH_NOT_FOUND);

  assert(mt_hash_set(&h, 1, 321) == 321);

  mt_array_free(data);

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

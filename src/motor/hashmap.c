#include "../../include/motor/hashmap.h"

#include <string.h>
#include "../../include/motor/arena.h"

static const uint64_t HASH_UNUSED = UINT64_MAX;

// TODO
// - resize the table when it's filled up

void mt_hash_init(MtHashMap *map, uint32_t size, MtArena *arena) {
  memset(map, 0, sizeof(*map));

  map->size  = size;
  map->arena = arena;

  map->keys   = mt_alloc(map->arena, sizeof(*map->keys) * map->size);
  map->values = mt_alloc(map->arena, sizeof(*map->values) * map->size);

  memset(map->keys, 0xff, sizeof(*map->keys) * map->size);
}

void mt_hash_clear(MtHashMap *map) {
  memset(map->keys, 0xff, sizeof(*map->keys) * map->size);
}

uint32_t mt_hash_set(MtHashMap *map, uint64_t key, uint32_t value) {
  uint32_t i     = key % map->size;
  uint32_t iters = 0;
  while (map->keys[i] != key && map->keys[i] != HASH_UNUSED &&
         iters < map->size) {
    i = (i + 1) % map->size;
    iters++;
  }

  if (iters >= map->size) {
    return MT_HASH_NOT_FOUND;
  }

  map->keys[i]   = key;
  map->values[i] = value;

  return value;
}

void mt_hash_remove(MtHashMap *map, uint64_t key) {
  uint32_t i     = key % map->size;
  uint32_t iters = 0;
  while (map->keys[i] != key && map->keys[i] != HASH_UNUSED &&
         iters < map->size) {
    i = (i + 1) % map->size;
    iters++;
  }

  if (iters >= map->size) {
    return;
  }

  map->keys[i] = HASH_UNUSED;

  return;
}

uint32_t mt_hash_get(MtHashMap *map, uint64_t key) {
  uint32_t i     = key % map->size;
  uint32_t iters = 0;
  while (map->keys[i] != key && map->keys[i] != HASH_UNUSED &&
         iters < map->size) {
    i = (i + 1) % map->size;
    iters++;
  }
  if (iters >= map->size) {
    return MT_HASH_NOT_FOUND;
  }

  return map->keys[i] == HASH_UNUSED ? MT_HASH_NOT_FOUND : map->values[i];
}

void mt_hash_destroy(MtHashMap *map) {
  mt_free(map->arena, map->keys);
  mt_free(map->arena, map->values);
}

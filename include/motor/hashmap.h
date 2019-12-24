#ifndef MT_HASHMAP
#define MT_HASHMAP

#include <inttypes.h>

#define MT_HASH_NOT_FOUND UINT32_MAX

typedef struct MtArena MtArena;

typedef struct MtHashMap {
  MtArena* arena;
  uint64_t *keys;
  uint32_t *values;
  uint32_t size;
} MtHashMap;

void mt_hash_init(MtHashMap *map, uint32_t size, MtArena* arena);

void mt_hash_clear(MtHashMap *map);

uint32_t mt_hash_set(MtHashMap *map, uint64_t key, uint32_t value);

void mt_hash_remove(MtHashMap *map, uint64_t key);

uint32_t mt_hash_get(MtHashMap *map, uint64_t key);

void mt_hash_destroy(MtHashMap *map);

#endif

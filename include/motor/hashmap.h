#pragma once

#include <inttypes.h>

#define MT_HASH_NOT_FOUND UINT64_MAX
#define MT_HASH_UNUSED UINT64_MAX

typedef struct MtArena MtArena;

typedef struct MtHashMap {
    MtArena *arena;
    uint64_t *keys;
    uint64_t *values;
    uintptr_t size;
} MtHashMap;

void mt_hash_init(MtHashMap *map, uint32_t size, MtArena *arena);

void mt_hash_clear(MtHashMap *map);

uintptr_t mt_hash_set(MtHashMap *map, uint64_t key, uintptr_t value);

void mt_hash_remove(MtHashMap *map, uint64_t key);

uintptr_t mt_hash_get(MtHashMap *map, uint64_t key);

void mt_hash_destroy(MtHashMap *map);

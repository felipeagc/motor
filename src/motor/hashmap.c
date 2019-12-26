#include "../../include/motor/hashmap.h"

#include <string.h>
#include "../../include/motor/arena.h"

static void hash_grow(MtHashMap *map) {
    uint32_t old_size     = map->size;
    uint64_t *old_keys    = map->keys;
    uintptr_t *old_values = map->values;

    map->size   = old_size * 2;
    map->keys   = mt_alloc(map->arena, sizeof(*map->keys) * map->size);
    map->values = mt_alloc(map->arena, sizeof(*map->values) * map->size);
    memset(map->keys, 0xff, sizeof(*map->keys) * map->size);

    for (uint32_t i = 0; i < old_size; i++) {
        if (old_keys[i] != MT_HASH_UNUSED) {
            mt_hash_set(map, old_keys[i], old_values[i]);
        }
    }

    mt_free(map->arena, old_keys);
    mt_free(map->arena, old_values);
}

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

uintptr_t mt_hash_set(MtHashMap *map, uint64_t key, uintptr_t value) {
    uint32_t i     = key % map->size;
    uint32_t iters = 0;
    while (map->keys[i] != key && map->keys[i] != MT_HASH_UNUSED &&
           iters < map->size) {
        i = (i + 1) % map->size;
        iters++;
    }

    if (iters >= map->size) {
        hash_grow(map);
        return mt_hash_set(map, key, value);
    }

    map->keys[i]   = key;
    map->values[i] = value;

    return value;
}

void mt_hash_remove(MtHashMap *map, uint64_t key) {
    uint32_t i     = key % map->size;
    uint32_t iters = 0;
    while (map->keys[i] != key && map->keys[i] != MT_HASH_UNUSED &&
           iters < map->size) {
        i = (i + 1) % map->size;
        iters++;
    }

    if (iters >= map->size) {
        return;
    }

    map->keys[i] = MT_HASH_UNUSED;

    return;
}

uintptr_t mt_hash_get(MtHashMap *map, uint64_t key) {
    uint32_t i     = key % map->size;
    uint32_t iters = 0;
    while (map->keys[i] != key && map->keys[i] != MT_HASH_UNUSED &&
           iters < map->size) {
        i = (i + 1) % map->size;
        iters++;
    }
    if (iters >= map->size) {
        return MT_HASH_NOT_FOUND;
    }

    return map->keys[i] == MT_HASH_UNUSED ? MT_HASH_NOT_FOUND : map->values[i];
}

void mt_hash_destroy(MtHashMap *map) {
    mt_free(map->arena, map->keys);
    mt_free(map->arena, map->values);
}

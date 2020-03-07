#include <motor/base/hashmap.h>

#include <string.h>
#include <motor/base/allocator.h>
#include "xxhash.h"

static void hash_grow(MtHashMap *map)
{
    uint32_t old_size     = map->size;
    uint64_t *old_keys    = map->keys;
    uint64_t *old_values = map->values;

    map->size   = old_size * 2;
    map->keys   = mt_alloc(map->alloc, sizeof(*map->keys) * map->size);
    map->values = mt_alloc(map->alloc, sizeof(*map->values) * map->size);
    memset(map->keys, 0xff, sizeof(*map->keys) * map->size);

    for (uint32_t i = 0; i < old_size; i++)
    {
        if (old_keys[i] != MT_HASH_UNUSED)
        {
            mt_hash_set_uint(map, old_keys[i], old_values[i]);
        }
    }

    mt_free(map->alloc, old_keys);
    mt_free(map->alloc, old_values);
}

uint64_t mt_hash_str(const char *str)
{
    return (uint64_t)XXH64(str, strlen(str), 0);
}

uint64_t mt_hash_strn(const char *str, size_t length)
{
    return (uint64_t)XXH64(str, length, 0);
}

void mt_hash_init(MtHashMap *map, uint32_t size, MtAllocator *alloc)
{
    memset(map, 0, sizeof(*map));

    map->size  = size;
    map->alloc = alloc;

    map->keys   = mt_alloc(map->alloc, sizeof(*map->keys) * map->size);
    map->values = mt_alloc(map->alloc, sizeof(*map->values) * map->size);

    memset(map->keys, 0xff, sizeof(*map->keys) * map->size);
}

void mt_hash_clear(MtHashMap *map)
{
    memset(map->keys, 0xff, sizeof(*map->keys) * map->size);
}

uint64_t mt_hash_set_uint(MtHashMap *map, uint64_t key, uint64_t value)
{
    uint32_t i     = key % map->size;
    uint32_t iters = 0;
    while (map->keys[i] != key && map->keys[i] != MT_HASH_UNUSED && iters < map->size)
    {
        i = (i + 1) % map->size;
        iters++;
    }

    if (iters >= map->size)
    {
        hash_grow(map);
        return mt_hash_set_uint(map, key, value);
    }

    map->keys[i]   = key;
    map->values[i] = value;

    return value;
}

uint64_t mt_hash_get_uint(MtHashMap *map, uint64_t key)
{
    uint32_t i     = key % map->size;
    uint32_t iters = 0;
    while (map->keys[i] != key && map->keys[i] != MT_HASH_UNUSED && iters < map->size)
    {
        i = (i + 1) % map->size;
        iters++;
    }
    if (iters >= map->size)
    {
        return MT_HASH_NOT_FOUND;
    }

    return map->keys[i] == MT_HASH_UNUSED ? MT_HASH_NOT_FOUND : map->values[i];
}

void *mt_hash_set_ptr(MtHashMap *map, uint64_t key, void *value)
{
    return (void *)mt_hash_set_uint(map, key, (uint64_t)value);
}

void *mt_hash_get_ptr(MtHashMap *map, uint64_t key)
{
    uint64_t result = mt_hash_get_uint(map, key);
    if (result == MT_HASH_NOT_FOUND)
        return NULL;
    return (void *)result;
}

void mt_hash_remove(MtHashMap *map, uint64_t key)
{
    uint32_t i     = key % map->size;
    uint32_t iters = 0;
    while (map->keys[i] != key && map->keys[i] != MT_HASH_UNUSED && iters < map->size)
    {
        i = (i + 1) % map->size;
        iters++;
    }

    if (iters >= map->size)
    {
        return;
    }

    map->keys[i] = MT_HASH_UNUSED;

    return;
}

void mt_hash_destroy(MtHashMap *map)
{
    mt_free(map->alloc, map->keys);
    mt_free(map->alloc, map->values);
}

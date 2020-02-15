#pragma once

#include "api_types.h"

#define MT_HASH_UNUSED UINTPTR_MAX
#define MT_HASH_NOT_FOUND UINTPTR_MAX

typedef struct MtAllocator MtAllocator;

typedef struct MtHashMap
{
    MtAllocator *alloc;
    uint64_t *keys;
    uintptr_t *values;
    uint32_t size;
} MtHashMap;

uint64_t mt_hash_str(const char *str);

uint64_t mt_hash_strn(const char *str, size_t length);

void mt_hash_init(MtHashMap *map, uint32_t size, MtAllocator *alloc);

void mt_hash_clear(MtHashMap *map);

uintptr_t mt_hash_set_uint(MtHashMap *map, uint64_t key, uintptr_t value);

uintptr_t mt_hash_get_uint(MtHashMap *map, uint64_t key);

void *mt_hash_set_ptr(MtHashMap *map, uint64_t key, void *value);

void *mt_hash_get_ptr(MtHashMap *map, uint64_t key);

void mt_hash_remove(MtHashMap *map, uint64_t key);

void mt_hash_destroy(MtHashMap *map);

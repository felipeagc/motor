#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MT_HASH_UNUSED UINT64_MAX
#define MT_HASH_NOT_FOUND UINT64_MAX

typedef struct MtAllocator MtAllocator;

typedef struct MtHashMap
{
    MtAllocator *alloc;
    uint64_t *keys;
    uint64_t *values;
    uint32_t size;
} MtHashMap;

MT_BASE_API uint64_t mt_hash_str(const char *str);

MT_BASE_API uint64_t mt_hash_strn(const char *str, size_t length);

MT_BASE_API void mt_hash_init(MtHashMap *map, uint32_t size, MtAllocator *alloc);

MT_BASE_API void mt_hash_clear(MtHashMap *map);

MT_BASE_API uint64_t mt_hash_set_uint(MtHashMap *map, uint64_t key, uint64_t value);

MT_BASE_API uint64_t mt_hash_get_uint(MtHashMap *map, uint64_t key);

MT_BASE_API void *mt_hash_set_ptr(MtHashMap *map, uint64_t key, void *value);

MT_BASE_API void *mt_hash_get_ptr(MtHashMap *map, uint64_t key);

MT_BASE_API void mt_hash_remove(MtHashMap *map, uint64_t key);

MT_BASE_API void mt_hash_destroy(MtHashMap *map);

#ifdef __cplusplus
}
#endif

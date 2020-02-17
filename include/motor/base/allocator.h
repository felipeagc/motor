#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator
{
    void *inst;
    void *(*realloc)(void *inst, void *ptr, uint64_t size);
} MtAllocator;

#define mt_alloc(alloc, size) mt_internal_alloc(alloc, size, __FILE__, __LINE__)

#define mt_realloc(alloc, ptr, size) mt_internal_realloc(alloc, ptr, size, __FILE__, __LINE__)

#define mt_free(alloc, ptr) mt_internal_free(alloc, ptr, __FILE__, __LINE__)

MT_BASE_API void *
mt_internal_alloc(MtAllocator *alloc, uint64_t size, const char *filename, uint32_t line);

MT_BASE_API void *mt_internal_realloc(
    MtAllocator *alloc, void *ptr, uint64_t size, const char *filename, uint32_t line);

MT_BASE_API void
mt_internal_free(MtAllocator *alloc, void *ptr, const char *filename, uint32_t line);

MT_BASE_API char *mt_strdup(MtAllocator *alloc, const char *str);

MT_BASE_API char *mt_strndup(MtAllocator *alloc, const char *str, uint64_t num_bytes);

MT_BASE_API char *mt_strcat(MtAllocator *alloc, char *dest, const char *src);

#ifdef __cplusplus
}
#endif

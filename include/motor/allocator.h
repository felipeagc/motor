#pragma once

#include <stdint.h>

typedef struct MtAllocator {
    void *inst;
    void *(*realloc)(void *inst, void *ptr, uint64_t size);
} MtAllocator;

#define mt_alloc(alloc, size) mt_internal_alloc(alloc, size, __FILE__, __LINE__)

#define mt_realloc(alloc, ptr, size)                                           \
    mt_internal_realloc(alloc, ptr, size, __FILE__, __LINE__)

#define mt_free(alloc, ptr) mt_internal_free(alloc, ptr, __FILE__, __LINE__)

void *mt_internal_alloc(
    MtAllocator *alloc, uint64_t size, const char *filename, uint32_t line);

void *mt_internal_realloc(
    MtAllocator *alloc,
    void *ptr,
    uint64_t size,
    const char *filename,
    uint32_t line);

void mt_internal_free(
    MtAllocator *alloc, void *ptr, const char *filename, uint32_t line);

char *mt_strdup(MtAllocator *alloc, char *str);

char *mt_strndup(MtAllocator *alloc, char *str, uint64_t num_bytes);

char *mt_strcat(MtAllocator *alloc, char *dest, const char* src);

#pragma once

#include <stdint.h>

typedef struct MtAllocator {
    void *inst;
    void *(*realloc)(void *inst, void *ptr, uint64_t size);
} MtAllocator;

#define mt_alloc(alloc, size) (alloc)->realloc((alloc)->inst, 0, size)

#define mt_realloc(alloc, ptr, size) (alloc)->realloc((alloc)->inst, ptr, size)

#define mt_free(alloc, ptr) (alloc)->realloc((alloc)->inst, ptr, 0)

char *mt_strdup(MtAllocator *alloc, char *str);

char *mt_strndup(MtAllocator *alloc, char *str, uint64_t num_bytes);

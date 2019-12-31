#pragma once

#include <stdint.h>

typedef struct MtAllocator {
    void *inst;
    void *(*realloc)(void *inst, void *ptr, uint64_t size);
} MtAllocator;

void *mt_alloc(MtAllocator *alloc, uint64_t size);

void *mt_calloc(MtAllocator *alloc, uint64_t size);

void *mt_realloc(MtAllocator *alloc, void *ptr, uint64_t size);

char *mt_strdup(MtAllocator *alloc, char *str);

char *mt_strndup(MtAllocator *alloc, char *str, uint64_t num_bytes);

void mt_free(MtAllocator *alloc, void *ptr);

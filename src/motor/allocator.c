#include "../../include/motor/allocator.h"

#include <string.h>

void *mt_alloc(MtAllocator *alloc, uint64_t size) {
    return alloc->realloc(alloc->inst, NULL, size);
}

void *mt_calloc(MtAllocator *alloc, uint64_t size) {
    void *ptr = alloc->realloc(alloc->inst, NULL, size);
    memset(ptr, 0, size);
    return ptr;
}

void *mt_realloc(MtAllocator *alloc, void *ptr, uint64_t size) {
    return alloc->realloc(alloc->inst, ptr, size);
}

char *mt_strdup(MtAllocator *alloc, char *str) {
    uint64_t len = strlen(str);
    char *s      = alloc->realloc(alloc->inst, NULL, len + 1);
    memcpy(s, str, len + 1);
    return s;
}

char *mt_strndup(MtAllocator *alloc, char *str, uint64_t num_bytes) {
    char *s = alloc->realloc(alloc->inst, NULL, num_bytes);
    strncpy(s, str, num_bytes);
    return s;
}

void mt_free(MtAllocator *alloc, void *ptr) {
    alloc->realloc(alloc->inst, ptr, 0);
}

#include "../../include/motor/allocator.h"

#include <string.h>

char *mt_strdup(MtAllocator *alloc, char *str) {
    uint64_t len = strlen(str);
    char *s      = mt_alloc(alloc, len + 1);
    memcpy(s, str, len + 1);
    return s;
}

char *mt_strndup(MtAllocator *alloc, char *str, uint64_t num_bytes) {
    char *s = mt_alloc(alloc, num_bytes);
    strncpy(s, str, num_bytes);
    return s;
}

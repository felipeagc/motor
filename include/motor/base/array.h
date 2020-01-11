#pragma once

#include <stdint.h>
#include <stdlib.h>

// The size of this is important to keep things aligned properly (16-byte alignment)
typedef struct MtArrayHeader
{
    uint64_t size;
    uint64_t capacity;
} MtArrayHeader;

typedef struct MtAllocator MtAllocator;

void *mt_array_grow(MtAllocator *alloc, void *a, uint64_t item_size, uint64_t cap);

#define mt_array_header(a) ((MtArrayHeader *)((char *)(a) - sizeof(MtArrayHeader)))

#define mt_array_size(a) ((a) ? mt_array_header(a)->size : 0)

#define mt_array_set_size(a, s) ((a) ? mt_array_header(a)->size = s : 0)

#define mt_array_capacity(a) ((a) ? mt_array_header(a)->capacity : 0)

#define mt_array_full(a) ((a) ? (mt_array_header(a)->size >= mt_array_header(a)->capacity) : 1)

#define mt_array_last(a) (&a[mt_array_size(a) - 1])

#define mt_array_push(alloc, a, item)                                                              \
    (mt_array_full(a) ? (a)          = mt_array_grow(alloc, a, sizeof(*a), 0) : 0,                 \
     (a)[mt_array_header(a)->size++] = (item))

#define mt_array_pop(a)                                                                            \
    (mt_array_size(a) > 0 ? (mt_array_header(a)->size--, &a[mt_array_size(a)]) : NULL)

#define mt_array_reserve(alloc, a, capacity)                                                       \
    (mt_array_full(a) ? a = mt_array_grow(alloc, a, sizeof(*a), capacity) : 0)

#define mt_array_add(alloc, a, count)                                                              \
    do                                                                                             \
    {                                                                                              \
        mt_array_reserve(alloc, a, mt_array_size(a) + count);                                      \
        mt_array_header(a)->size = (mt_array_size(a) + count);                                     \
    } while (0)

#define mt_array_add_zeroed(alloc, a, count)                                                       \
    do                                                                                             \
    {                                                                                              \
        mt_array_reserve(alloc, a, mt_array_size(a) + count);                                      \
        memset(a + mt_array_size(a), 0, sizeof(*a) * count);                                       \
        mt_array_header(a)->size = (mt_array_size(a) + count);                                     \
    } while (0)

#define mt_array_free(alloc, a) ((a) ? mt_free(alloc, mt_array_header(a)) : 0)

#define mt_array_foreach(item, a) for (item = (a); item != (a) + mt_array_size(a); ++item)

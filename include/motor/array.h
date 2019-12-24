#ifndef MT_ARRAY
#define MT_ARRAY

#include <inttypes.h>
#include <stdlib.h>

enum { MT_ARRAY_INITIAL_CAPACITY = 16 };

typedef struct MtArrayHeader {
  uint32_t size;
  uint32_t capacity;
} MtArrayHeader;

#define mt_array_header(a)                                                     \
  ((MtArrayHeader *)((char *)(a) - sizeof(MtArrayHeader)))

#define mt_array_size(a) ((a) ? mt_array_header(a)->size : 0)

#define mt_array_capacity(a) ((a) ? mt_array_header(a)->capacity : 0)

#define mt_array_full(a)                                                       \
  ((a) ? (mt_array_header(a)->size >= mt_array_header(a)->capacity) : 1)

static void *mt_array_grow(void *a, uint32_t item_size, uint32_t cap) {
  if (!a) {
    uint32_t desired_cap = ((cap == 0) ? MT_ARRAY_INITIAL_CAPACITY : cap);

    a = ((char *)malloc(sizeof(MtArrayHeader) + (item_size * desired_cap))) +
        sizeof(MtArrayHeader);
    mt_array_header(a)->size     = 0;
    mt_array_header(a)->capacity = desired_cap;
    return a;
  }

  uint32_t desired_cap =
      ((cap == 0) ? (mt_array_header(a)->capacity * 2) : cap);
  mt_array_header(a)->capacity = desired_cap;
  return ((char *)realloc(
             mt_array_header(a),
             sizeof(MtArrayHeader) + (desired_cap * item_size))) +
         sizeof(MtArrayHeader);
}

#define mt_array_push(a, item)                                                 \
  (mt_array_full(a) ? a          = mt_array_grow(a, sizeof(*a), 0) : 0,        \
   a[mt_array_header(a)->size++] = (item),                                     \
   mt_array_header(a)->size - 1)

#define mt_array_reserve(a, capacity)                                          \
  (mt_array_full(a) ? a = mt_array_grow(a, sizeof(*a), capacity) : 0)

#define mt_array_free(a) ((a) ? free(mt_array_header(a)) : 0)

#endif

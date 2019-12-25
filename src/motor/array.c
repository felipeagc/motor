#include "../../include/motor/array.h"

#include "../../include/motor/arena.h"

void *mt_array_grow(MtArena *arena, void *a, uint32_t item_size, uint32_t cap) {
  if (!a) {
    uint32_t desired_cap = ((cap == 0) ? MT_ARRAY_INITIAL_CAPACITY : cap);

    a = ((char *)mt_alloc(
            arena, sizeof(MtArrayHeader) + (item_size * desired_cap))) +
        sizeof(MtArrayHeader);
    mt_array_header(a)->size     = 0;
    mt_array_header(a)->capacity = desired_cap;
    return a;
  }

  uint32_t desired_cap =
      ((cap == 0) ? (mt_array_header(a)->capacity * 2) : cap);
  mt_array_header(a)->capacity = desired_cap;
  return ((char *)mt_realloc(
             arena,
             mt_array_header(a),
             sizeof(MtArrayHeader) + (desired_cap * item_size))) +
         sizeof(MtArrayHeader);
}

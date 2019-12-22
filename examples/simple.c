#include "arena.h"
#include "renderer.h"

int main(int argc, char *argv[]) {
  MtArena arena;
  mt_arena_init(&arena, 1 << 14);

  MtDevice device = mt_device_create(
      &arena, &(MtDeviceDescriptor){.flags = MT_DEVICE_HEADLESS});

  mt_device_destroy(device);

  mt_arena_destroy(&arena);
  return 0;
}

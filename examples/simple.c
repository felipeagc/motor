#include "arena.h"
#include "renderer.h"
#include "threads.h"
#include "window.h"
#include "vulkan/vulkan_device.h"
#include "vulkan/glfw_window.h"

int main(int argc, char *argv[]) {
  MtArena arena;
  mt_arena_init(&arena, 1 << 14);

  MtVulkanWindowSystem *window_system = mt_glfw_vulkan_init();

  MtIDevice device;
  mt_vulkan_device_init(
      &device,
      &(MtVulkanDeviceDescriptor){.window_system = window_system},
      &arena);

  MtIWindow window;
  mt_glfw_vulkan_window_init(&window, 800, 600, "Hello", &arena);

  window.vt->destroy(window.inst);

  device.vt->destroy(device.inst);

  mt_glfw_vulkan_destroy();
  mt_arena_destroy(&arena);
  return 0;
}

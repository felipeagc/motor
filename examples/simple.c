#include <motor/arena.h>
#include <motor/renderer.h>
#include <motor/threads.h>
#include <motor/window.h>
#include <motor/vulkan/vulkan_device.h>
#include <motor/vulkan/glfw_window.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  MtArena arena;
  mt_arena_init(&arena, 1 << 14);

  MtIWindowSystem window_system;
  mt_glfw_vulkan_init(&window_system);

  MtIDevice device;
  mt_vulkan_device_init(
      &device,
      &(MtVulkanDeviceDescriptor){.window_system = &window_system},
      &arena);

  MtIWindow window;
  mt_glfw_vulkan_window_init(&window, &device, 800, 600, "Hello", &arena);

  while (!window.vt->should_close(window.inst)) {
    window_system.vt->poll_events();

    MtEvent event;
    while (window.vt->next_event(window.inst, &event)) {
      switch (event.type) {
      case MT_EVENT_WINDOW_CLOSED: {
        printf("Closed\n");
      } break;
      }
    }

    MtICmdBuffer cb = window.vt->begin_frame(window.inst);

    cb.vt->begin(cb.inst);

    cb.vt->begin_render_pass(cb.inst, window.vt->get_render_pass(window.inst));

    cb.vt->end_render_pass(cb.inst);

    cb.vt->end(cb.inst);

    window.vt->end_frame(window.inst);
  }

  window.vt->destroy(window.inst);
  device.vt->destroy(device.inst);
  window_system.vt->destroy();

  mt_arena_destroy(&arena);
  return 0;
}

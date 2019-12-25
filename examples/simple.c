#include <motor/arena.h>
#include <motor/renderer.h>
#include <motor/threads.h>
#include <motor/window.h>
#include <motor/vulkan/vulkan_device.h>
#include <motor/vulkan/glfw_window.h>
#include <stdio.h>
#include <assert.h>

static uint8_t *load_shader(MtArena *arena, const char *path, size_t *size) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *code = mt_alloc(arena, *size);
  fread(code, *size, 1, f);

  fclose(f);

  return code;
}

int main(int argc, char *argv[]) {
  MtArena arena;
  mt_arena_init(&arena, 1 << 14);

  MtIWindowSystem window_system;
  mt_glfw_vulkan_init(&window_system);

  MtIDevice dev;
  mt_vulkan_device_init(
      &dev,
      &(MtVulkanDeviceDescriptor){.window_system = &window_system},
      &arena);

  MtIWindow window;
  mt_glfw_vulkan_window_init(&window, &dev, 800, 600, "Hello", &arena);

  uint8_t *vertex_code = NULL, *fragment_code = NULL;
  size_t vertex_code_size = 0, fragment_code_size = 0;

  vertex_code =
      load_shader(&arena, "../shaders/test.vert.spv", &vertex_code_size);
  assert(vertex_code);
  fragment_code =
      load_shader(&arena, "../shaders/test.frag.spv", &fragment_code_size);
  assert(fragment_code);

  MtPipeline *pipeline = dev.vt->create_graphics_pipeline(
      dev.inst,
      vertex_code,
      vertex_code_size,
      fragment_code,
      fragment_code_size,
      &(MtGraphicsPipelineDescriptor){
          .blending    = false,
          .depth_test  = false,
          .depth_write = false,
          .depth_bias  = false,
          .cull_mode   = MT_CULL_MODE_NONE,
          .front_face  = MT_FRONT_FACE_COUNTER_CLOCKWISE,
          .line_width  = 1.0f,
          .vertex_descriptor =
              {
                  .attributes      = NULL,
                  .attribute_count = 0,
                  .stride          = 0,
              },
      });

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

    cb.vt->bind_pipeline(cb.inst, pipeline);

    cb.vt->end_render_pass(cb.inst);

    cb.vt->end(cb.inst);

    window.vt->end_frame(window.inst);
  }

  dev.vt->destroy_pipeline(dev.inst, pipeline);

  window.vt->destroy(window.inst);
  dev.vt->destroy(dev.inst);
  window_system.vt->destroy();

  mt_arena_destroy(&arena);
  return 0;
}

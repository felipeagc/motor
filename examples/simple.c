#include <motor/arena.h>
#include <motor/renderer.h>
#include <motor/threads.h>
#include <motor/window.h>
#include <motor/util.h>
#include <motor/math_types.h>
#include <motor/vulkan/vulkan_device.h>
#include <motor/vulkan/glfw_window.h>
#include <stdio.h>
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

typedef struct Vertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 tex_coords;
} Vertex;

int main(int argc, char *argv[]) {
    MtArena arena;
    mt_arena_init(&arena, 1 << 14);

    MtIWindowSystem window_system;
    mt_glfw_vulkan_init(&window_system);

    MtDevice *dev = mt_vulkan_device_init(
        &(MtVulkanDeviceCreateInfo){.window_system = &window_system}, &arena);

    MtIWindow window;
    mt_glfw_vulkan_window_init(&window, dev, 800, 600, "Hello", &arena);

    stbi_set_flip_vertically_on_load(true);
    int32_t w, h, num_channels;
    uint8_t *image_data =
        stbi_load("../assets/test.png", &w, &h, &num_channels, 4);

    MtImage *image = mt_render.create_image(
        dev,
        &(MtImageCreateInfo){
            .width  = (uint32_t)w,
            .height = (uint32_t)h,
            .format = MT_FORMAT_RGBA8_UNORM,
        });

    mt_render.transfer_to_image(
        dev,
        &(MtImageCopyView){.image = image},
        (uint32_t)(num_channels * w * h),
        image_data);

    free(image_data);

    MtSampler *sampler = mt_render.create_sampler(
        dev,
        &(MtSamplerCreateInfo){.min_filter = MT_FILTER_NEAREST,
                               .mag_filter = MT_FILTER_NEAREST});

    uint8_t *vertex_code = NULL, *fragment_code = NULL;
    size_t vertex_code_size = 0, fragment_code_size = 0;

    vertex_code = load_shader(
        &arena, "../shaders/build/test.vert.spv", &vertex_code_size);
    assert(vertex_code);
    fragment_code = load_shader(
        &arena, "../shaders/build/test.frag.spv", &fragment_code_size);
    assert(fragment_code);

    MtPipeline *pipeline = mt_render.create_graphics_pipeline(
        dev,
        vertex_code,
        vertex_code_size,
        fragment_code,
        fragment_code_size,
        &(MtGraphicsPipelineCreateInfo){
            .cull_mode  = MT_CULL_MODE_NONE,
            .front_face = MT_FRONT_FACE_COUNTER_CLOCKWISE,
            .vertex_attributes =
                (const MtVertexAttribute[]){
                    {.format = MT_FORMAT_RGB32_SFLOAT, .offset = 0},
                    {.format = MT_FORMAT_RGB32_SFLOAT,
                     .offset = sizeof(float) * 3},
                    {.format = MT_FORMAT_RG32_SFLOAT,
                     .offset = sizeof(float) * 6}},
            .vertex_attribute_count = 3,
            .vertex_stride          = sizeof(Vertex),
        });

    Vertex vertices[4] = {
        {{0.5f, -0.5f, 0.0f}, {}, {1.0f, 1.0f}},  // Top right
        {{0.5f, 0.5f, 0.0f}, {}, {1.0f, 0.0f}},   // Bottom right
        {{-0.5f, 0.5f, 0.0f}, {}, {0.0f, 0.0f}},  // Bottom left
        {{-0.5f, -0.5f, 0.0f}, {}, {0.0f, 1.0f}}, // Top left
    };

    uint16_t indices[6] = {2, 1, 0, 0, 3, 2};

    while (!window.vt->should_close(window.inst)) {
        mt_render.device_begin_frame(dev);

        window_system.vt->poll_events();

        MtEvent event;
        while (window.vt->next_event(window.inst, &event)) {
            switch (event.type) {
            case MT_EVENT_WINDOW_CLOSED: {
                printf("Closed\n");
            } break;
            }
        }

        MtCmdBuffer *cb = window.vt->begin_frame(window.inst);

        mt_render.begin_cmd_buffer(cb);

        mt_render.cmd_begin_render_pass(
            cb, window.vt->get_render_pass(window.inst));

        mt_render.cmd_bind_pipeline(cb, pipeline);
        mt_render.cmd_bind_vertex_data(cb, vertices, sizeof(vertices));
        mt_render.cmd_bind_index_data(
            cb, indices, sizeof(indices), MT_INDEX_TYPE_UINT16);
        mt_render.cmd_bind_image(cb, image, sampler, 0, 0);
        mt_render.cmd_bind_uniform(
            cb, &(Vec3){1.0f, 1.0f, 1.0f}, sizeof(Vec3), 0, 1);
        mt_render.cmd_draw_indexed(cb, MT_LENGTH(indices), 1);

        mt_render.cmd_end_render_pass(cb);

        mt_render.end_cmd_buffer(cb);

        window.vt->end_frame(window.inst);
    }

    mt_render.destroy_image(dev, image);
    mt_render.destroy_sampler(dev, sampler);

    mt_render.destroy_pipeline(dev, pipeline);

    window.vt->destroy(window.inst);
    mt_render.destroy_device(dev);
    window_system.vt->destroy();

    mt_arena_destroy(&arena);
    return 0;
}

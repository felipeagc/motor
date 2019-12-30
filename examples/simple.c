#include <motor/arena.h>
#include <motor/renderer.h>
#include <motor/threads.h>
#include <motor/window.h>
#include <motor/util.h>
#include <motor/math_types.h>
#include <motor/file_watcher.h>
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

static const char *VERT_PATH = "../shaders/build/test.vert.spv";
static const char *FRAG_PATH = "../shaders/build/test.frag.spv";

typedef struct Game {
    MtArena arena;
    MtIWindowSystem window_system;
    MtIWindow window;
    MtDevice *dev;

    MtFileWatcher *watcher;

    MtImage *image;
    MtSampler *sampler;
    MtPipeline *pipeline;
} Game;

void game_init(Game *g) {
    mt_arena_init(&g->arena, 1 << 16);

    mt_glfw_vulkan_init(&g->window_system);

    g->dev = mt_vulkan_device_init(
        &(MtVulkanDeviceCreateInfo){.window_system = &g->window_system},
        &g->arena);

    mt_glfw_vulkan_window_init(
        &g->window, g->dev, 800, 600, "Hello", &g->arena);

    g->watcher = mt_file_watcher_create(
        &g->arena, MT_FILE_WATCHER_EVENT_MODIFY, "../shaders");

    stbi_set_flip_vertically_on_load(true);
    int32_t w, h, num_channels;
    uint8_t *image_data =
        stbi_load("../assets/test.png", &w, &h, &num_channels, 4);

    g->image = mt_render.create_image(
        g->dev,
        &(MtImageCreateInfo){
            .width  = (uint32_t)w,
            .height = (uint32_t)h,
            .format = MT_FORMAT_RGBA8_UNORM,
        });

    mt_render.transfer_to_image(
        g->dev,
        &(MtImageCopyView){.image = g->image},
        (uint32_t)(num_channels * w * h),
        image_data);

    free(image_data);

    g->sampler = mt_render.create_sampler(
        g->dev,
        &(MtSamplerCreateInfo){.min_filter = MT_FILTER_NEAREST,
                               .mag_filter = MT_FILTER_NEAREST});

    uint8_t *vertex_code = NULL, *fragment_code = NULL;
    size_t vertex_code_size = 0, fragment_code_size = 0;

    vertex_code = load_shader(&g->arena, VERT_PATH, &vertex_code_size);
    assert(vertex_code);
    fragment_code = load_shader(&g->arena, FRAG_PATH, &fragment_code_size);
    assert(fragment_code);

    g->pipeline = mt_render.create_graphics_pipeline(
        g->dev,
        vertex_code,
        vertex_code_size,
        fragment_code,
        fragment_code_size,
        &(MtGraphicsPipelineCreateInfo){
            .cull_mode  = MT_CULL_MODE_NONE,
            .front_face = MT_FRONT_FACE_COUNTER_CLOCKWISE,
            .vertex_attributes =
                (MtVertexAttribute[]){
                    {.format = MT_FORMAT_RGB32_SFLOAT, .offset = 0},
                    {.format = MT_FORMAT_RGB32_SFLOAT,
                     .offset = sizeof(float) * 3},
                    {.format = MT_FORMAT_RG32_SFLOAT,
                     .offset = sizeof(float) * 6}},
            .vertex_attribute_count = 3,
            .vertex_stride          = sizeof(Vertex),
        });

    mt_free(&g->arena, vertex_code);
    mt_free(&g->arena, fragment_code);
}

void game_destroy(Game *g) {
    mt_render.destroy_image(g->dev, g->image);
    mt_render.destroy_sampler(g->dev, g->sampler);

    mt_render.destroy_pipeline(g->dev, g->pipeline);

    g->window.vt->destroy(g->window.inst);
    mt_render.destroy_device(g->dev);
    g->window_system.vt->destroy();

    mt_file_watcher_destroy(g->watcher);

    mt_arena_destroy(&g->arena);
}

void recreate_pipeline(Game *g) {
    mt_render.destroy_pipeline(g->dev, g->pipeline);

    uint8_t *vertex_code = NULL, *fragment_code = NULL;
    size_t vertex_code_size = 0, fragment_code_size = 0;

    vertex_code = load_shader(&g->arena, VERT_PATH, &vertex_code_size);
    assert(vertex_code);
    fragment_code = load_shader(&g->arena, FRAG_PATH, &fragment_code_size);
    assert(fragment_code);

    g->pipeline = mt_render.create_graphics_pipeline(
        g->dev,
        vertex_code,
        vertex_code_size,
        fragment_code,
        fragment_code_size,
        &(MtGraphicsPipelineCreateInfo){
            .cull_mode  = MT_CULL_MODE_NONE,
            .front_face = MT_FRONT_FACE_COUNTER_CLOCKWISE,
            .vertex_attributes =
                (MtVertexAttribute[]){
                    {.format = MT_FORMAT_RGB32_SFLOAT, .offset = 0},
                    {.format = MT_FORMAT_RGB32_SFLOAT,
                     .offset = sizeof(float) * 3},
                    {.format = MT_FORMAT_RG32_SFLOAT,
                     .offset = sizeof(float) * 6}},
            .vertex_attribute_count = 3,
            .vertex_stride          = sizeof(Vertex),
        });

    mt_free(&g->arena, vertex_code);
    mt_free(&g->arena, fragment_code);
}

int main(int argc, char *argv[]) {
    Game game = {0};
    game_init(&game);

    Vertex vertices[4] = {
        {{0.5f, -0.5f, 0.0f}, {}, {1.0f, 1.0f}},  // Top right
        {{0.5f, 0.5f, 0.0f}, {}, {1.0f, 0.0f}},   // Bottom right
        {{-0.5f, 0.5f, 0.0f}, {}, {0.0f, 0.0f}},  // Bottom left
        {{-0.5f, -0.5f, 0.0f}, {}, {0.0f, 1.0f}}, // Top left
    };

    uint16_t indices[6] = {2, 1, 0, 0, 3, 2};

    while (!game.window.vt->should_close(game.window.inst)) {
        MtFileWatcherEvent e;
        while (mt_file_watcher_poll(game.watcher, &e)) {
            switch (e.type) {
            case MT_FILE_WATCHER_EVENT_MODIFY: {
                if (strcmp(e.src, FRAG_PATH) == 0 ||
                    strcmp(e.src, FRAG_PATH) == 0) {
                    recreate_pipeline(&game);
                }
            } break;
            }
        }

        game.window_system.vt->poll_events();

        MtEvent event;
        while (game.window.vt->next_event(game.window.inst, &event)) {
            switch (event.type) {
            case MT_EVENT_WINDOW_CLOSED: {
                printf("Closed\n");
            } break;
            }
        }

        MtCmdBuffer *cb = game.window.vt->begin_frame(game.window.inst);

        mt_render.begin_cmd_buffer(cb);

        mt_render.cmd_begin_render_pass(
            cb, game.window.vt->get_render_pass(game.window.inst));

        mt_render.cmd_bind_pipeline(cb, game.pipeline);
        mt_render.cmd_bind_vertex_data(cb, vertices, sizeof(vertices));
        mt_render.cmd_bind_index_data(
            cb, indices, sizeof(indices), MT_INDEX_TYPE_UINT16);
        mt_render.cmd_bind_image(cb, game.image, game.sampler, 0, 0);
        mt_render.cmd_bind_uniform(cb, &V3(1, 1, 1), sizeof(Vec3), 0, 1);
        mt_render.cmd_draw_indexed(cb, MT_LENGTH(indices), 1);

        mt_render.cmd_end_render_pass(cb);

        mt_render.end_cmd_buffer(cb);

        game.window.vt->end_frame(game.window.inst);
    }

    game_destroy(&game);

    return 0;
}

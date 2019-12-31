#include <motor/allocator.h>
#include <motor/arena.h>
#include <motor/renderer.h>
#include <motor/threads.h>
#include <motor/window.h>
#include <motor/util.h>
#include <motor/math_types.h>
#include <motor/file_watcher.h>
#include <motor/engine.h>
#include <motor/assets/pipeline_asset.h>
#include <motor/assets/image_asset.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

static uint8_t *
load_shader(MtAllocator *alloc, const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *code = mt_alloc(alloc, *size);
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
    MtEngine engine;

    MtFileWatcher *watcher;

    MtImageAsset *image_asset;
    MtPipeline *pipeline;
} Game;

void game_init(Game *g) {
    mt_engine_init(&g->engine);

    g->image_asset = (MtImageAsset *)mt_image_asset_create(
                         &g->engine.asset_manager, "../assets/test.png")
                         ->inst;

    g->watcher = mt_file_watcher_create(
        &g->engine.alloc, MT_FILE_WATCHER_EVENT_MODIFY, "../shaders");

    uint8_t *vertex_code = NULL, *fragment_code = NULL;
    size_t vertex_code_size = 0, fragment_code_size = 0;

    vertex_code = load_shader(&g->engine.alloc, VERT_PATH, &vertex_code_size);
    assert(vertex_code);
    fragment_code =
        load_shader(&g->engine.alloc, FRAG_PATH, &fragment_code_size);
    assert(fragment_code);

    g->pipeline = mt_render.create_graphics_pipeline(
        g->engine.device,
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

    mt_free(&g->engine.alloc, vertex_code);
    mt_free(&g->engine.alloc, fragment_code);
}

void game_destroy(Game *g) {
    mt_render.destroy_pipeline(g->engine.device, g->pipeline);

    mt_file_watcher_destroy(g->watcher);

    mt_engine_destroy(&g->engine);
}

void recreate_pipeline(Game *g) {
    mt_render.destroy_pipeline(g->engine.device, g->pipeline);

    uint8_t *vertex_code = NULL, *fragment_code = NULL;
    size_t vertex_code_size = 0, fragment_code_size = 0;

    vertex_code = load_shader(&g->engine.alloc, VERT_PATH, &vertex_code_size);
    assert(vertex_code);
    fragment_code =
        load_shader(&g->engine.alloc, FRAG_PATH, &fragment_code_size);
    assert(fragment_code);

    g->pipeline = mt_render.create_graphics_pipeline(
        g->engine.device,
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

    mt_free(&g->engine.alloc, vertex_code);
    mt_free(&g->engine.alloc, fragment_code);
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

    while (!game.engine.window.vt->should_close(game.engine.window.inst)) {
        MtFileWatcherEvent e;
        while (mt_file_watcher_poll(game.watcher, &e)) {
            switch (e.type) {
            case MT_FILE_WATCHER_EVENT_MODIFY: {
                if (strcmp(e.src, FRAG_PATH) == 0 ||
                    strcmp(e.src, FRAG_PATH) == 0) {
                    recreate_pipeline(&game);
                }
            } break;
            default: break;
            }
        }

        game.engine.window_system.vt->poll_events();

        MtEvent event;
        while (game.engine.window.vt->next_event(
            game.engine.window.inst, &event)) {
            switch (event.type) {
            case MT_EVENT_WINDOW_CLOSED: {
                printf("Closed\n");
            } break;
            default: break;
            }
        }

        MtCmdBuffer *cb =
            game.engine.window.vt->begin_frame(game.engine.window.inst);

        mt_render.begin_cmd_buffer(cb);

        mt_render.cmd_begin_render_pass(
            cb,
            game.engine.window.vt->get_render_pass(game.engine.window.inst));

        mt_render.cmd_bind_pipeline(cb, game.pipeline);
        mt_render.cmd_bind_vertex_data(cb, vertices, sizeof(vertices));
        mt_render.cmd_bind_index_data(
            cb, indices, sizeof(indices), MT_INDEX_TYPE_UINT16);
        mt_render.cmd_bind_image(
            cb, game.image_asset->image, game.image_asset->sampler, 0, 0);
        mt_render.cmd_bind_uniform(cb, &V3(1, 1, 1), sizeof(Vec3), 0, 1);
        mt_render.cmd_draw_indexed(cb, MT_LENGTH(indices), 1);

        mt_render.cmd_end_render_pass(cb);

        mt_render.end_cmd_buffer(cb);

        game.engine.window.vt->end_frame(game.engine.window.inst);
    }

    game_destroy(&game);

    return 0;
}

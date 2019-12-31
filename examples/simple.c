#include <motor/allocator.h>
#include <motor/renderer.h>
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

typedef struct Vertex {
    Vec3 pos;
    Vec3 normal;
    Vec2 tex_coords;
} Vertex;

typedef struct Game {
    MtEngine engine;

    MtFileWatcher *watcher;

    MtImageAsset *image_asset;
    MtPipelineAsset *pipeline_asset;
} Game;

void game_init(Game *g) {
    mt_engine_init(&g->engine);

    g->watcher = mt_file_watcher_create(
        g->engine.alloc, MT_FILE_WATCHER_EVENT_MODIFY, "../assets");

    g->image_asset = (MtImageAsset *)mt_asset_manager_load(
        &g->engine.asset_manager, "../assets/test.png");

    g->pipeline_asset = (MtPipelineAsset *)mt_asset_manager_load(
        &g->engine.asset_manager, "../assets/shaders/test.glsl");
    assert(g->pipeline_asset);
}

void game_destroy(Game *g) {
    mt_file_watcher_destroy(g->watcher);

    mt_engine_destroy(&g->engine);
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
                mt_asset_manager_load(&game.engine.asset_manager, e.src);
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

        mt_render.cmd_bind_pipeline(cb, game.pipeline_asset->pipeline);
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

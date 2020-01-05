#include <motor/base/allocator.h>
#include <motor/base/util.h>
#include <motor/base/math.h>
#include <motor/base/array.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/window.h>
#include <motor/engine/ui.h>
#include <motor/engine/file_watcher.h>
#include <motor/engine/engine.h>
#include <motor/engine/camera.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <motor/engine/assets/image_asset.h>
#include <motor/engine/assets/font_asset.h>
#include <motor/engine/assets/gltf_asset.h>
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

    MtUIRenderer *ui;

    MtFileWatcher *watcher;
    MtPerspectiveCamera cam;

    MtImageAsset *image;
    MtFontAsset *font;
    MtPipelineAsset *model_pipeline;
    MtGltfAsset *model;
} Game;

void game_init(Game *g) {
    memset(g, 0, sizeof(*g));

    mt_engine_init(&g->engine);

    g->ui = mt_ui_create(g->engine.alloc, &g->engine.asset_manager);

    g->watcher = mt_file_watcher_create(
        g->engine.alloc, MT_FILE_WATCHER_EVENT_MODIFY, "../assets");

    mt_perspective_camera_init(&g->cam);

    g->image = (MtImageAsset *)mt_asset_manager_load(
        &g->engine.asset_manager, "../assets/test.png");
    assert(g->image);

    g->model_pipeline = (MtPipelineAsset *)mt_asset_manager_load(
        &g->engine.asset_manager, "../assets/shaders/model.glsl");
    assert(g->model_pipeline);

    g->font = (MtFontAsset *)mt_asset_manager_load(
        &g->engine.asset_manager, "../assets/fonts/PTSerif-BoldItalic.ttf");
    assert(g->font);

    g->model = (MtGltfAsset *)mt_asset_manager_load(
        &g->engine.asset_manager, "../assets/DamagedHelmet.glb");
    assert(g->model);
}

void game_destroy(Game *g) {
    mt_file_watcher_destroy(g->watcher);
    mt_ui_destroy(g->ui);

    mt_engine_destroy(&g->engine);
}

void asset_watcher_handler(MtFileWatcherEvent *e, void *user_data) {
    Game *g = (Game *)user_data;

    switch (e->type) {
    case MT_FILE_WATCHER_EVENT_MODIFY: {
        mt_asset_manager_load(&g->engine.asset_manager, e->src);
    } break;
    default: break;
    }
}

int main(int argc, char *argv[]) {
    Game game = {0};
    game_init(&game);

    MtIWindow *win = &game.engine.window;

    while (!win->vt->should_close(win->inst)) {
        mt_file_watcher_poll(game.watcher, asset_watcher_handler, &game);

        game.engine.window_system.vt->poll_events();

        MtEvent event;
        while (win->vt->next_event(win->inst, &event)) {
            mt_perspective_camera_on_event(&game.cam, &event);
            switch (event.type) {
            case MT_EVENT_WINDOW_CLOSED: {
                printf("Closed\n");
            } break;
            default: break;
            }
        }

        MtCmdBuffer *cb = win->vt->begin_frame(win->inst);

        mt_render.begin_cmd_buffer(cb);

        mt_render.cmd_begin_render_pass(
            cb, win->vt->get_render_pass(win->inst));

        // Draw UI
        {
            MtViewport viewport;
            mt_render.cmd_get_viewport(cb, &viewport);
            mt_ui_begin(game.ui, &viewport);

            mt_ui_set_font(game.ui, game.font);
            mt_ui_set_font_size(game.ui, 50);

            mt_ui_printf(
                game.ui, "Delta: %fms", win->vt->delta_time(win->inst));

            mt_ui_draw(game.ui, cb);
        }

        uint32_t width, height;
        win->vt->get_size(win->inst, &width, &height);
        float aspect = (float)width / (float)height;
        mt_perspective_camera_update(&game.cam, win, aspect);

        // Draw model
        {
            Mat4 transform = mat4_identity();
            mt_render.cmd_bind_pipeline(cb, game.model_pipeline->pipeline);
            mt_render.cmd_bind_uniform(
                cb, &game.cam.uniform, sizeof(game.cam.uniform), 0, 0);
            mt_gltf_asset_draw(game.model, cb, &transform, 1);
        }

        mt_render.cmd_end_render_pass(cb);

        mt_render.end_cmd_buffer(cb);

        win->vt->end_frame(win->inst);
    }

    game_destroy(&game);

    return 0;
}

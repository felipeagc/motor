#include <motor/allocator.h>
#include <motor/renderer.h>
#include <motor/window.h>
#include <motor/util.h>
#include <motor/math.h>
#include <motor/file_watcher.h>
#include <motor/engine.h>
#include <motor/array.h>
#include <motor/ui.h>
#include <motor/assets/pipeline_asset.h>
#include <motor/assets/image_asset.h>
#include <motor/assets/font_asset.h>
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

    MtImageAsset *image;
    MtFontAsset *font;
    MtPipelineAsset *text_pipeline;
} Game;

void game_init(Game *g) {
    memset(g, 0, sizeof(*g));

    mt_engine_init(&g->engine);

    g->ui = mt_ui_create(g->engine.alloc, &g->engine.asset_manager);

    g->watcher = mt_file_watcher_create(
        g->engine.alloc, MT_FILE_WATCHER_EVENT_MODIFY, "../assets");

    g->image = (MtImageAsset *)mt_asset_manager_load(
        &g->engine.asset_manager, "../assets/test.png");
    assert(g->image);

    g->font = (MtFontAsset *)mt_asset_manager_load(
        &g->engine.asset_manager, "../assets/fonts/PTSerif-BoldItalic.ttf");
    assert(g->font);
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

    while (!game.engine.window.vt->should_close(game.engine.window.inst)) {
        mt_file_watcher_poll(game.watcher, asset_watcher_handler, &game);

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

        // Draw UI
        {
            MtViewport viewport;
            mt_render.cmd_get_viewport(cb, &viewport);
            mt_ui_begin(game.ui, &viewport);

            mt_ui_set_font(game.ui, game.font);
            mt_ui_set_font_size(game.ui, 50);

            mt_ui_text(game.ui, "Hello World");
            mt_ui_set_color(game.ui, V3(1, 0, 0));
            mt_ui_text(game.ui, "Hello");
            mt_ui_set_color(game.ui, V3(0, 1, 0));
            mt_ui_text(game.ui, "Hello");
            mt_ui_set_color(game.ui, V3(0, 0, 1));
            mt_ui_text(game.ui, "Hello");

            mt_ui_draw(game.ui, cb);
        }

        mt_render.cmd_end_render_pass(cb);

        mt_render.end_cmd_buffer(cb);

        game.engine.window.vt->end_frame(game.engine.window.inst);
    }

    game_destroy(&game);

    return 0;
}

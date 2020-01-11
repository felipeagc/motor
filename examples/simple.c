#include <motor/base/allocator.h>
#include <motor/base/util.h>
#include <motor/base/math.h>
#include <motor/base/array.h>
#include <motor/base/thread_pool.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/window.h>
#include <motor/engine/ui.h>
#include <motor/engine/file_watcher.h>
#include <motor/engine/engine.h>
#include <motor/engine/camera.h>
#include <motor/engine/environment.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <motor/engine/assets/image_asset.h>
#include <motor/engine/assets/font_asset.h>
#include <motor/engine/assets/gltf_asset.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct Game
{
    MtEngine engine;

    MtUIRenderer *ui;

    MtFileWatcher *watcher;
    MtPerspectiveCamera cam;

    MtImageAsset *image;
    MtFontAsset *font;

    MtPipelineAsset *model_pipeline;

    MtMutex models_mutex;
    MtGltfAsset **models;
    Mat4 *transforms;

    MtEnvironment env;

    MtThreadPool thread_pool;
} Game;

typedef struct AssetLoadInfo
{
    Game *g;
    const char *path;
    float scale;
} AssetLoadInfo;

static int32_t asset_load(void *arg)
{
    AssetLoadInfo *info = arg;
    Game *g             = info->g;
    const char *path    = info->path;
    float scale         = info->scale;
    mt_free(g->engine.alloc, info);

    MtAsset *asset = mt_asset_manager_load(&g->engine.asset_manager, path);

    mt_mutex_lock(&g->models_mutex);

    mt_array_push(g->engine.alloc, g->models, (MtGltfAsset *)asset);
    MtGltfAsset **model = mt_array_last(g->models);

    assert(*model);

    Mat4 transform = mat4_identity();
    transform      = mat4_scale(transform, V3(scale, scale, scale));
    transform =
        mat4_translate(transform, V3((float)(mt_array_size(g->models) - 1) * 5.0f, 0.0f, 0.0f));
    mt_array_push(g->engine.alloc, g->transforms, transform);

    mt_mutex_unlock(&g->models_mutex);

    return 0;
}

void load_model(Game *g, const char *path, float scale)
{
    AssetLoadInfo *info = mt_alloc(g->engine.alloc, sizeof(AssetLoadInfo));

    info->g     = g;
    info->path  = path;
    info->scale = scale;

    mt_thread_pool_enqueue(&g->thread_pool, asset_load, info);
}

void game_init(Game *g)
{
    memset(g, 0, sizeof(*g));

    uint32_t num_threads = mt_cpu_count() / 2;
    printf("Using %u threads\n", num_threads);

    mt_engine_init(&g->engine, num_threads);

    mt_thread_pool_init(&g->thread_pool, num_threads, g->engine.alloc);
    mt_mutex_init(&g->models_mutex);

    g->ui = mt_ui_create(g->engine.alloc, &g->engine.asset_manager);

    g->watcher = mt_file_watcher_create(g->engine.alloc, MT_FILE_WATCHER_EVENT_MODIFY, "../assets");

    mt_perspective_camera_init(&g->cam);

    g->image =
        (MtImageAsset *)mt_asset_manager_load(&g->engine.asset_manager, "../assets/test.png");
    assert(g->image);

    g->model_pipeline = (MtPipelineAsset *)mt_asset_manager_load(
        &g->engine.asset_manager, "../assets/shaders/pbr.glsl");
    assert(g->model_pipeline);

    g->font = (MtFontAsset *)mt_asset_manager_load(
        &g->engine.asset_manager, "../assets/fonts/PTSerif-BoldItalic.ttf");
    assert(g->font);

    load_model(g, "../assets/BoomBox.glb", 100.0f);
    /* load_model(g, "../assets/Lantern.glb", 0.2f); */
    /* load_model(g, "../assets/NormalTangentMirrorTest.glb", 1.0f); */
    /* load_model(g, "../assets/both.glb", 0.1f); */
    load_model(g, "../assets/helmet_ktx.glb", 1.0f);

    MtImageAsset *skybox_asset = (MtImageAsset *)mt_asset_manager_load(
        &g->engine.asset_manager, "../assets/papermill_hdr16f_cube.ktx");

    mt_environment_init(&g->env, &g->engine.asset_manager, skybox_asset);

    mt_thread_pool_wait_all(&g->thread_pool);
}

void game_destroy(Game *g)
{
    mt_array_free(g->engine.alloc, g->models);
    mt_array_free(g->engine.alloc, g->transforms);

    mt_file_watcher_destroy(g->watcher);
    mt_ui_destroy(g->ui);
    mt_environment_destroy(&g->env);

    mt_thread_pool_destroy(&g->thread_pool);
    mt_mutex_destroy(&g->models_mutex);

    mt_engine_destroy(&g->engine);
}

void asset_watcher_handler(MtFileWatcherEvent *e, void *user_data)
{
    Game *g = (Game *)user_data;

    switch (e->type)
    {
        case MT_FILE_WATCHER_EVENT_MODIFY:
        {
            mt_asset_manager_load(&g->engine.asset_manager, e->src);
            break;
        }
        default: break;
    }
}

int main(int argc, char *argv[])
{
    Game game = {0};
    game_init(&game);

    MtWindow *win = game.engine.window;

    while (!mt_window.should_close(win))
    {
        mt_file_watcher_poll(game.watcher, asset_watcher_handler, &game);

        mt_window.poll_events();

        MtEvent event;
        while (mt_window.next_event(win, &event))
        {
            mt_perspective_camera_on_event(&game.cam, &event);
            switch (event.type)
            {
                case MT_EVENT_WINDOW_CLOSED:
                {
                    printf("Closed\n");
                    break;
                }
                default: break;
            }
        }

        MtCmdBuffer *cb = mt_window.begin_frame(win);

        mt_render.begin_cmd_buffer(cb);

        mt_render.cmd_begin_render_pass(cb, mt_window.get_render_pass(win));

        MtViewport viewport;
        mt_render.cmd_get_viewport(cb, &viewport);
        mt_ui_begin(game.ui, &viewport);

        mt_ui_set_font(game.ui, game.font);
        mt_ui_set_font_size(game.ui, 50);

        float delta_time = mt_window.delta_time(win);
        mt_ui_printf(game.ui, "Delta: %fms", delta_time);

        uint32_t width, height;
        mt_window.get_size(win, &width, &height);
        float aspect = (float)width / (float)height;
        mt_perspective_camera_update(&game.cam, win, aspect);

        mt_ui_printf(
            game.ui,
            "Pos: %.2f  %.2f  %.2f",
            game.cam.uniform.pos.x,
            game.cam.uniform.pos.y,
            game.cam.uniform.pos.z);

        // Draw skybox
        {
            mt_render.cmd_bind_uniform(cb, &game.cam.uniform, sizeof(game.cam.uniform), 0, 0);
            mt_environment_draw_skybox(&game.env, cb);
        }

        // Draw model
        {
            static float angle = 0.0f;

            angle += delta_time;

            mt_render.cmd_bind_pipeline(cb, game.model_pipeline->pipeline);
            mt_render.cmd_bind_uniform(cb, &game.cam.uniform, sizeof(game.cam.uniform), 0, 0);
            mt_environment_bind(&game.env, cb, 2);

            for (uint32_t i = 0; i < mt_array_size(game.models); i++)
            {
                mt_gltf_asset_draw(game.models[i], cb, &game.transforms[i], 1, 3);
            }
        }

        mt_ui_draw(game.ui, cb);

        mt_render.cmd_end_render_pass(cb);

        mt_render.end_cmd_buffer(cb);

        mt_window.end_frame(win);
    }

    game_destroy(&game);

    return 0;
}
